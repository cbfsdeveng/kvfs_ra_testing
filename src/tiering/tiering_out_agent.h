/*
 * Tiering-out-agent.
 * The tiering agent deals with moving data to and from the key-value-store
 * (KVS) underlying our filesystem.
 * This part, the tiering *out* agent, stages the data to local persistent
 * storage, which is then moved out to the cloud via a cloud-proxy.
 * Lifetime: as long as the filesystem is mounted and log entries are being
 * generated, ToA should be running.
 */
#ifndef TIERING_TIERING_OUT_AGENT_
#define TIERING_TIERING_OUT_AGENT_


#include "cloud_proxy.h"
#include "fs_change_log.h"
#include "index_manager.h"
#include "kvfs_mount.h"
#include "segment.h"
#include "snapshot_persister.h"
#include <thread>
#include <unordered_map>

namespace pqfs {
// Each instance of TieringOutAgent is meant to work on a specific mount.
// Once started, run in the background until stopped.
class TieringOutAgent {
public:
    //
    TieringOutAgent(
            KvfsMount* kvfs_mount,  // not owned.
            FsChangeLog* fs_change_log,  // not owned. property of filesystem?
            CloudProxy* cloud_proxy,  // not owned.
            IndexManager* index_manager,
            SegmentFactory* segment_factory
            ) : kvfs_mount(kvfs_mount),
                fs_change_log(fs_change_log),
                cloud_proxy(cloud_proxy),
                index_manager(index_manager),
                segment_factory(segment_factory),
                run_thread(nullptr),
                stop_requested(false),
                snapshot_requested(false) {
    }
    // set various options
    int SetOptions(std::string options);
    // something more sophisticated here re. threads.
    int Start();

    // These functions allow external control of the TOA.
    int RequestStop();
    void RequestSnapshot() {
	// TODO: synchronization.
        snapshot_requested = true;
    }
    // Wait until transactions up to xid have been flushed, up to millis ms.
    xid_t Wait(xid_t flushed_to_xid, int32_t millis = 0);
    void Join();

    FsChangeLog* get_fs_change_log() const { return fs_change_log; }

    void AddFsId(uint64_t fs_id) {
        fs_ids.push_back(fs_id);
    }
    // (event_string == "/DUMP")
    void DumpKVStore(std::ostringstream& result);
    // (event_string == "/DBSTATS")
    void DBStats(std::ostringstream& result);
private:
    KvfsMount* kvfs_mount;  // not owned.
    FsChangeLog* fs_change_log;  // not owned. property of filesystem?
    CloudProxy* cloud_proxy;  // not owned.
    IndexManager* index_manager;  // not owned.
    SegmentFactory* segment_factory;  // not owned.
    // Just a single working thread for now.
    std::thread* run_thread;

    volatile bool stop_requested;
    volatile bool snapshot_requested;

    // List of FSID's we're watching.
    // For now, we only support 1.
    // Still need to decide whether we have just one TOA for all, or one per FS,
    // or ... Likewise, decide if one big log or per-fs log.
    std::vector<uint64_t> fs_ids;

    // Tiering Out Agent's main run loop (runs in its own thread).
    void RunInternal();

    // Synchronously perform a filesystem snapshot.
    // (create tarballs, update and publish index).
    void Snapshot();

    void GetPaths(
            std::unordered_map<uint64_t, std::string>* paths,
            const std::vector<uint64_t>& inode_nums);

    Status GetPath(uint64_t inode, std::string* pathp);

};

// Helper: get the deduped list of inodes from a vector of log ents.
void GetInodes(
        std::vector<uint64_t>* inode_nums,  // Out.
        const std::vector<proto::ChangeLogEntry> log_entries);


}  // pqfs
#endif // TIERING_TIERING_OUT_AGENT_
