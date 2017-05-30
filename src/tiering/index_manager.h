
/*
 * Manages our in-core and local on-disk copy of the index.
 * The index describes (which parts of) which inodes are contained in which
 * segments (aka tarballs).
 * Index format is specified as a proto.
 * On startup, the old index is read in from disk.
 * When a new segment (tarball) is committed, the index manager is asked to
 * update the in-core index with info from the new segment.
 * That index may then be written back to disk, replacing the old copy.
 * (note that this must be done atomically, so we actually write a new index,
 * then do a rename, or just find the most recently named index file).
 */
#ifndef TIERING_INDEX_MANAGER_H_
#define  TIERING_INDEX_MANAGER_H_

#include <string>
#include "index.pb.h"
#include "cloud_proxy.h"
#include "fs_change_log.h"

namespace pqfs {

class Segment;

class IndexManager {
  public:
    enum IndexFormat {
        INVALID_FORMAT = 0,
        TEXT_FORMAT = 1,        // During development - readable, slow
        BINARY_FORMAT = 2       // Not human readable, very fast.
    };

    IndexManager(
            CloudProxy* cloud_proxy,
            IndexFormat index_format);

    // Re-read the index from disk.
    // Returns 0 or errno.
    int Refresh(bool merge = false);

    // Update the in-core index with inode list from this segment
    // (so index will reflect that this segment (tarball) contains those
    // inodes).
    // Returns 0 or errno.
    int Update(const Segment* segment);

    // Adds a segment to the index and returns a mutable pointer.
    // Index retains ownership of the segment.
    proto::Segment* AddSegment();

    // Return segment containing the latest copy of the specified file.
    // The segment returned is owned by the index. No release op for now.
    const proto::Segment* GetSegment(
            int64_t inode_num,
            xid_t before_xid,  // xid of this version of inode <= before_xid.
            proto::InodeOffset* inode_off_ret) // offset or path of inode.
                const;

    // Commit the incore version of the index to stable storage.
    int Flush();

    std::string DebugString() {
        return "dir " + cloud_proxy->get_local_dir() +
            " format " + std::to_string(index_format) +
            index.DebugString();
    }

  private:
    CloudProxy* cloud_proxy;  // To get and put the persistent index. Not owned.
    const IndexFormat index_format;

    proto::Index index;
};

}  // pqfs

#endif  // TIERING_INDEX_MANAGER_H_
