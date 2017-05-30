/*
 * Tiering-in-agent.
 * The tiering agent deals with pulling file info and data back from
 * snapshots (when it has been evicted, or when a file is mounted for the
 * first time on a new KVS.
 */
#ifndef TIERING_TIERING_IN_AGENT_
#define TIERING_TIERING_IN_AGENT_


#include "kvfs_mount.h"
#include "segment.h"
#include <thread>
#include <unordered_map>

namespace pqfs {

class IndexManager;
class CloudProxy;
class SegmentFactory;

class TieredInFile;

// TODO: Various status operations might be useful.
// 
class TieringInAgent {
public:
    //
    TieringInAgent(
            IndexManager* index_manager,    // not owned.
            CloudProxy* cloud_proxy,        // not owned.
            SegmentFactory* segment_factory // not owned.
            ) : index_manager(index_manager),
                cloud_proxy(cloud_proxy),
                segment_factory(segment_factory),
                run_thread(nullptr),
                stop_requested(false) {
    }
    // set various options
    int SetOptions(std::string options);

    TieredInFile* Open(uint64_t inode_num, xid_t min_xid);
    void Close(TieredInFile*);

    // something more sophisticated here re. threads.
    // Thread model for this? If requests block too long,
    // are requests interruptible?
    // TODO: define abstract class with the thread stuff and helpers.
    int Start();

    int RequestStop();

    // After RequestStop, Join waits for worker thread to exit.
    void Join();

private:
    IndexManager* index_manager;  // not owned.
    CloudProxy* cloud_proxy;  // not owned.
    SegmentFactory* segment_factory; // not owned.

    // Just a single working thread for now.
    std::thread* run_thread; // OWNED

    volatile bool stop_requested;

    // Tiering In Agent's main run loop (runs in its own thread).
    // (not sure we need a thread...)
    void RunInternal();
};

}  // namespace pqfs
#endif // TIERING_TIERING_IN_AGENT_
