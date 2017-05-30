
#include "snapshot_persister.h"

#include "index_manager.h"
#include "segment.h"
#include <google/protobuf/text_format.h>
#include <glog/logging.h>


#include <assert.h>
#include <stdio.h>

#include <chrono>
#include <thread>

#include <algorithm>
#include <set>
#include <vector>

using google::protobuf::TextFormat;

namespace pqfs {

class InodeByPathComparator {
  public:
    InodeByPathComparator(
            std::unordered_map<uint64_t,
            std::string>& paths) :
        paths(paths) {
    }
    bool operator() (uint64_t i, int64_t j) {
        const std::string& path_i = paths[i];
        const std::string& path_j = paths[j];
        return (path_i.compare(path_j) < 0); 
    }
  private:
    std::unordered_map<uint64_t, std::string>& paths;
};

int SnapshotPersister::Snapshot(
        SegmentFactory* segment_factory,
        IndexManager* index_manager,
        std::vector<uint64_t>* inode_nums,
        xid_t first_xid,
        xid_t last_xid) {
    int error = 0;
    Segment* segment = nullptr;
    // Get the path for each inode.
    // TODO(joe): potentially large for all-in-memory.
    std::unordered_map<uint64_t, std::string> paths;
    GetPaths(&paths, inode_nums);
    if (paths.size() == 0) {
        // current changes must all have been moot (files deleted).
        VLOG(1) << "Snapshot: paths empty";
        error = ENOENT;
    }
    if (error == 0) {
        InodeByPathComparator path_comparator(paths);
        std::sort(inode_nums->begin(), inode_nums->end(), path_comparator);
        VLOG(1) << "Snapshot: try to add " << inode_nums->size()
            << " inodes";
        segment = segment_factory->AddSegment(first_xid, last_xid);
        for (auto inode : *inode_nums) {
            auto path_iter = paths.find(inode);
            if (path_iter != paths.end()) {
                // For now, grab entire file (metadata + data).
                // TODO: consider what op(s) are involved to see what's
                // necessary.
                segment->Add(inode, path_iter->second,
                        Segment::WHOLE_FILE);
            } else {
                LOG(ERROR) << "Snapshot inode " << inode << " name not found.";
            }
        }
        if (segment->Commit() == 0) {
            index_manager->Update(segment);
            index_manager->Flush();
            cloud_proxy->Sync(pqfs::CloudProxy::SYNC_OUT);
        }
        segment->Close();
        delete segment;
    }
    return error;
}

// TODO: Move to segment to access snapshot's path info!
// ### Move to snapshot-persister.
void
SnapshotPersister::GetPaths(
        std::unordered_map<uint64_t, std::string>* paths,
        const std::vector<uint64_t>* inode_nums) {
    // For each inode num:
    //   path = [];
    //   do {
    //     ask filesystem for list of names and parent inodes for this inode
    //     (from base object).
    //     pick one name/inode pair
    //     insert name into path[]
    //     inode num = parent's num.
    //   } while (node_num valid & != root);
    //   inode_nums[inum] = path;
    //
    // Could build a cache of paths for directory inodes (class PathBuilder...)
    for (auto inode : *inode_nums) {
        std::string path;
        Status status = GetPath(inode, &path);
        if (status == STATUS_OK) {
            (*paths)[inode] = path;
        } else {
            // This happens when file has been deleted (compression failure due
            // to segment boundaries).
            VLOG(1) << "GetPaths i " << inode << " path " << path
                << " status " << status;
        }
    }
    if (inode_nums->size() == 0) {
        VLOG(1) << "GetPaths: got " << paths->size()
                << " paths for " << inode_nums->size() + " inodes.";
    }
}

Status
SnapshotPersister::GetPath(uint64_t inode, std::string* pathp) {
    // std::string("i#" + std::to_string(inode));
    uint64_t parent_handle(0);
    pathp->clear();
    std::vector<std::string> path_parts;  // simple, inefficient path building.

    Status status = STATUS_OK;
    while (inode > FileSystemManager::ROOT_HANDLE) {
        std::string path_part;
        status = snapshot_fs_manager->getPathInfo(
                0, inode, &parent_handle, &path_part);
        VLOG(2) << "GetPath: getPathInfo status " << status
            << " parent " << parent_handle << " name '" << path_part << "'";
        if (status != STATUS_OK) {
            break;
        }
        inode = parent_handle;
        path_parts.insert(path_parts.begin(), path_part);
    }
    for (auto part : path_parts) {
        pathp->append(part);
        pathp->append("/");
    }
    // trim trailing slash.
    if (pathp->size() > 0) {
        pathp->pop_back();
    }
    assert(pathp->size() > 0 || inode == FileSystemManager::ROOT_HANDLE
            || status != STATUS_OK);
    VLOG(2) << "GetPath: inode " << inode << " path '" << *pathp
        << "' status " << status;
    return status;
}

int SnapshotPersister::Close() {
    snapshot_fs_manager->UnmountSnapshot();
    return 0;
}

} // namespace pqfs
