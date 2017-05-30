/*
 * Eviction Agent.
 * Evict inodes to keep our KVS space usage in check.
 */
#ifndef TIERING_EVICTION_AGENT_
#define TIERING_EVICTION_AGENT_


#include "FileSystemManager.h"
#include "fs_change_log.h"
#include <assert.h>
#include <thread>

namespace pqfs {

class TieringOutAgent;  // unfortunately, circular dependency.
// Each instance of TieringOutAgent is meant to work on a specific filesystem.
// Once started, they run in the background until stopped.

class EvictionAgent {
public:
    //
    EvictionAgent(
        KvfsMount* kvfs_mount,          // not owned. Used to stat and evict inodes.
        FsChangeLog* fs_change_log // not owned
    ) : kvfs_mount(kvfs_mount),
        fs_change_log(fs_change_log),
        stop_requested(false),
        eviction_requested(false),
        run_thread(nullptr)
    {
    }
    // Tiering Out Agent's main run loop (runs in its own thread).
    // Evict tiered-out inodes until space-used is below threshold.
    // (Public for now. May make private when/if eviction agent has own thread.
    void ProcessEvictions();

    int Start();

    int RequestStop();

    void Join();

    void RequestEviction() {
        // TODO: synchronization.
        eviction_requested = true;
    }

    xid_t get_eviction_next_xid() {
        return fs_change_log->get_eviction_next_xid();
    }

private:
    // Wait for eviction request, process it, repeat.
    void RunInternal();

    void CompactDatabase();

    EvictionAgent();

    KvfsMount* kvfs_mount;
    FsChangeLog* fs_change_log;

    volatile bool stop_requested;
    volatile bool eviction_requested;

    std::thread* run_thread;
};

}  // pqfs
#endif // TIERING_EVICTION_AGENT_
