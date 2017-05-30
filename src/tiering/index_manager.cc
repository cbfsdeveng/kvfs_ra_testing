/*
 * Index Manager implementation.
 */

#include "index_manager.h"

#include <fcntl.h>
//#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include "segment.h"
#include <stdio.h>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

using namespace google::protobuf;
using namespace google::protobuf::io;

namespace pqfs {
IndexManager::IndexManager(
            CloudProxy* cloud_proxy,
            IndexFormat index_format) :
    cloud_proxy(cloud_proxy),
    index_format(index_format) {
    index.set_magic(17);   // NEEDSWORK: placeholder.
    index.set_version(1);
}

// Serialize the in-core index to disk via protocol buffer magic.
int IndexManager::Flush() {
    int ret = 0;
    std::string index_path(cloud_proxy->LocalPath(
                index_format == TEXT_FORMAT
                ? "index_new.ix.txt"
                : "index_new.ix.bin"));
    LOG(INFO) << "IndexManager::Flush() to " + index_path;
    int fd = creat(index_path.c_str(), 0777);
    if (fd < 0) {
        ret = errno ? errno : EIO;
    } else {
        switch (index_format) {
        case TEXT_FORMAT:
        {
            FileOutputStream ostream(fd);
            if (!TextFormat::Print(index, &ostream)) {
                ret = errno ? errno : EIO;
            }
            break;
        }
        case BINARY_FORMAT:
            if (!index.SerializeToFileDescriptor(fd)) {
                ret = errno ? errno : EIO;
            }
            break;
        default:
            ret = EINVAL;
            break;
        }
    }
    if (ret) {
        LOG(ERROR) << "IndexManager::Flush err " << ret << " "  << index_path;
    }
    close(fd);  // harmless to close -1
    return ret;
}

// Re-read the index from disk.
int IndexManager::Refresh(bool merge) {
    int ret = 0;
    LOG(INFO) << "IndexManager::Refresh()";
    std::string index_path(cloud_proxy->LocalPath(
                index_format == TEXT_FORMAT
                ? "index_new.ix.txt"
                : "index_new.ix.bin"));
    int fd = open(index_path.c_str(), 0);
    if (fd < 0) {
        ret = errno ? errno : EIO;
    } else {
        if (!merge) {
            // NEEDSWORK: Not ideal that an error during refresh could delete
            // our current index (would be bad).
            index.Clear();
        }
        switch (index_format) {
        case TEXT_FORMAT:
        {
            FileInputStream istream(fd);
            if (!TextFormat::Merge(&istream, &index)) {
                ret = errno ? errno : EIO;
            }
            break;
        }
        case BINARY_FORMAT:
            if (merge) {
                // TODO: in merge case, would parse into, then merge from a new
                // index, since MergeFromFileDescriptor doesn't exist (grr). Not
                // used yet anyway.
                LOG(FATAL) << "IndexManager::Refresh binary merge NYI.";
            }
            if (!index.ParseFromFileDescriptor(fd)) {
                ret = errno ? errno : EIO;
            }
            break;
        default:
            ret = EINVAL;
            break;
        }
    }
    return ret;
}

// Adds a segment to the index and returns a mutable pointer.
// Index retains ownership of the segment.
// TODO: retain a lock until done with the returned segment.
proto::Segment* IndexManager::AddSegment() {
    return index.add_segment();
}

const proto::Segment* IndexManager::GetSegment(
        int64_t inode_num, xid_t before_xid,
        proto::InodeOffset* inode_off_ret) const {
    // Scan through indexed segments in reverse xid order (recent to ancient).
    // We'll find the first (most recent) segment that has target inode.
    for (int seg = index.segment_size(); seg-- > 0; ) {
        const pqfs::proto::Segment& segment = index.segment(seg);
        if (segment.last_xid() > before_xid) {
            VLOG(3) << "IndexManager::GetSegment skipping seg " << seg
                    << " last_xid " << segment.last_xid()
                    << " before_xid " << before_xid;
            continue;
        }
        for (int i = 0; i < segment.inode_offset_size(); i++) {
            if (segment.inode_offset(i).inode() == inode_num) {
                VLOG(0) << "IndexManager::GetSegment inode " << inode_num
                        << " seg " << segment.DebugString()
                        << " i_off " << segment.inode_offset(i).DebugString();
                *inode_off_ret = segment.inode_offset(i);
                // lifetime: segment is inside the index.
                return &segment;
            }
        }
    }
    // Should not be possible.
    LOG(ERROR)  << "IndexManager:GetSegment - inode " << inode_num
                << " not found!";
    return nullptr;
}

// Update the in-core index with inode list from this segment
// (so index will reflect that this segment (tarball) contains those inodes).
// Returns 0 or errno.
int IndexManager::Update(const Segment* segment) {
    LOG(INFO) << "IndexManager::Update()";
    // Get the list of inodes from the segment.
    const std::map<int64_t, std::string>& inode_paths(
            segment->get_inode_paths());
    // Add a proto::Segment to the index.
    proto::Segment* proto_segment = index.add_segment();
    proto_segment->set_name(segment->get_name());
    proto_segment->set_first_xid(segment->get_first_xid());
    proto_segment->set_last_xid(segment->get_last_xid());
    // Add those inodes to the new proto::Segment.
    for (auto inode_path : inode_paths) {
	proto::InodeOffset* inode_offset = proto_segment->add_inode_offset();
	inode_offset->set_inode(inode_path.first);
        inode_offset->set_pathname(inode_path.second);
	inode_offset->set_offset(0L);  // placeholder for clarity - not required.
        DVLOG(2) << "Update add " << inode_offset->DebugString();
    }
    return 0;
}

}  // pqfs
