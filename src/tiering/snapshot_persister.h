/*
 * Snapshot-persister.
 * Once instantiated with a filesytem manager and a proxy, allows a set of inodes
 * to be persisted as segments (tarballs) via a cloud-proxy.
 */
#ifndef SNAPSHOT_PERSISTER_
#define SNAPSHOT_PERSISTER_

#include "FileSystemManager.h"
#include "cloud_proxy.h"
#include "index_manager.h"
#include "segment.h"
#include <thread>
#include <unordered_map>

namespace pqfs {

// Each instance of SnapshotPersister is meant to work on a specific filesystem.
// For now, lifetime is a single snapshot.
class SnapshotPersister {
public:
    //
    SnapshotPersister(
            FileSystemManager* snapshot_fs_manager,  // OWNED.
            CloudProxy* proxy  // not owned.
            ) : snapshot_fs_manager(snapshot_fs_manager),
                cloud_proxy(proxy) {
    }

    // TODO(joe): named snapshots?
    // Perform a snapshot using specified segment factory and index_manager to create
    // tarballs, update and publish index.
    // Note: Note that inode_nums is NOT const here (to avoid copy).
    // Returns 0 on success, ENOENT if snapshot was empty (hence no tarballs
    // created, no index update), other error codes if snapshot creation fails.
    // ### flush status - currently Snapshot is synchronous - on successful return,
    // tarballs have been created and synced to the cloud.
    // ### list of tarballs(??) - isn't this in the index? List of tarballs created by a single
    // Snapshot could be maintained by segment_factory(?).
    // On return, the index has been updated and written to the proxy.
    int Snapshot(
            SegmentFactory* segment_factory,
            IndexManager* index_manager,
            std::vector<uint64_t>* inode_nums,
            xid_t first_xid,
            xid_t last_xid);

    int Close();

private:
    std::unique_ptr<FileSystemManager> snapshot_fs_manager;  // OWNED
    CloudProxy* cloud_proxy;  // not owned.

    void GetPaths(
            std::unordered_map<uint64_t, std::string>* paths,
            const std::vector<uint64_t>* inode_nums);

    Status GetPath(uint64_t inode, std::string* pathp);

};

}  // pqfs
#endif // SNAPSHOT_PERSISTER_
