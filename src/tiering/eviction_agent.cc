#include "eviction_agent.h"
#include "tiering_out_agent.h"
#include "index_manager.h"
#include "WallTime.h"

#include <assert.h>
#include <stdio.h>

#include <chrono>
#include <thread>

namespace pqfs {


/**
 * See:
 *  https://github.com/ray1967/kvfs/wiki/Notes-on-Tiering-Agent
 */



int EvictionAgent::Start() {
    if (run_thread != nullptr) {
        return EINVAL;
    }
    // starts running immediately.
    stop_requested = false;
    run_thread = new std::thread(&EvictionAgent::RunInternal, this);
    return 0;
}

int EvictionAgent::RequestStop() {
    // NEEDSWORK: synchronize, notify.
    stop_requested = true;
    return 0;
}

void EvictionAgent::Join() {
    assert(run_thread);
    run_thread->join();
    delete run_thread;
    run_thread = nullptr;
}

void EvictionAgent::RunInternal() {
    do {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        VLOG(4) << "RunInternal woke up";
        if (eviction_requested) {
            ProcessEvictions();
            eviction_requested = false;
        }
    } while (!stop_requested);
}

// Although tiering-out and eviction are linked (you can't evict what
// hasn't been tiered-out), they shouldn't really interact much. Their shared
// information is really just TOA's "I've teired out to this point in the log".
// Other than that, eviction just works from the log.
void EvictionAgent::ProcessEvictions()
{
//  https://github.com/ray1967/kvfs/wiki/Notes-on-Tiering-Agent
// (Algorithm from Mark's "Notes on Tiering Agent" in kvfs wiki)
// Walk through the log up until tiered_out_xid.
// for each entry,
//   for each inode in entry
//     if xid in inode is newer than xid in this entry,
//       skip it - we will see inode again later in the log.
//     if any inode time is "recent", or space used is small (or we're not
//         low on space? - if so, why in this loop??),
//       add a "spared inode I as of xid X" entry at front of log.
// (Note that when we fault back in, we must add a spared entry so we'll
// consider it later).
// Once eviction processing has scanned a segment of the log file, we no
// longer need to keep it in-core. We can either push it out to cloud (as we
// do now), or delete after some time.

    // TODO: loop over the read-evict processing below, if we limit the # xids.
    std::vector<proto::ChangeLogEntry> log_entries;
    xid_t first_xid = fs_change_log->get_eviction_next_xid();
    xid_t last_xid =  fs_change_log->get_tiered_out_xid();
    // how many to read?
    // Note that xid's in change_log are strictly increasing, but not
    // necessarily contiguous.
    // TODO(): consider limiting last_xid here.
    LOG(INFO) << "ProcessEvictions: read first_xid " << first_xid
            << " last_xid " << last_xid;

    // ###
    // amount_to_evict = kvfs-space-used - kvfs-space-max;
    // if (amount_to_evict <= 0)
    //    return;

    fs_change_log->Read(&log_entries, &first_xid, last_xid);
    LOG(INFO) << "ProcessEvictions: read " << log_entries.size()
            << " entries";
    // Get the list of unique inodes in this batch of changes.
    // BUG? If I extract the inodes from cl instead of walking entries one at a time, 
    // how do I ensure I don't skip entries when I set_eviction_next_xid?
    // maybe sort the inodes by max-xid-for-that-inode?
    // For now (per Mark), don't break - run algorithm for everything in batcgh of xid's.
    std::vector<uint64_t> inode_nums;
    GetInodes(&inode_nums, /*const&*/log_entries);
    if (log_entries.size() > 0) {
        last_xid = log_entries.back().xid();
        log_entries.back().has_xid();
    }
    LOG(INFO) << "ProcessEvictions: got " << inode_nums.size() << " inodes";
    for (auto inode_num : inode_nums) {
        LOG(INFO) << "ProcessEvictions: consider inode " << inode_num;
        // Stat the file, then copy it out to disk.
        struct stat stat;
        FuseKvfsInode* inodep = kvfs_mount->GetInode(inode_num, true,
                "Eviction");
        int err = inodep->Getattr(&stat);
        if (err) {
            // When we find log entries for files that have
            // been deleted since they were modified.
            LOG(INFO) << "ProcessEvictions: inode " << inode_num
                    << " failed " << err;
            continue;
        }
        if (S_ISDIR(stat.st_mode)) {
            // for now we're not going to evict dirs.
            LOG(INFO) << "ProcessEvictions: skip dir i# " << inode_num;
        } else if (S_ISREG(stat.st_mode)) {
            LOG(INFO) << "ProcessEvictions: consider file i# " << inode_num
                    << " st_size " << stat.st_size;
            time_t now = WallTime::secondsTimestampToUnix(
                    WallTime::secondsTimestamp());
            // TODO: eviction age should be arg to evict pass, or a local
            // config param.
            // NEEDSWORK: For now, very short for testing.
            time_t eviction_age = 2;
            if ((now - stat.st_mtime) >= eviction_age) {
                int evict_ret = inodep->Evict();
                LOG(INFO) << "ProcessEviction: evict inode " << inode_num
                        << " age " << (now - stat.st_mtime) << "s"
                        << " ret " << evict_ret;
            }
        } else {
            LOG(ERROR) << "ProcessEviction: st_mode " << std::oct
                    << stat.st_mode << " not supported";
        }
        // stat the inode (filesystem?) via tiering-out-agent's filesystem.
        // if (!exists)
        //     continue;
        // if (inode change or mod time is recent) {
            // insert spared entry at front of log (via tiering-agent).
        //     continue;
        // }
        // evict inode via toa.
        // increment amount-evicted by size evicted.
        // TODO(): maybe we can't break here? (due to eviction_next_xid).
    }
    // Compact the database now that we've removed some values.
    // TODO(): probably not most efficient way. leveldb allows us to compact
    // each file separately (by compacting its range of keys).
    CompactDatabase();
    LOG(INFO) << "ProcessEvictions: set_eviction_next " << last_xid + 1;
    fs_change_log->set_eviction_next_xid(last_xid + 1);
}

void EvictionAgent::CompactDatabase() {
    auto* store = kvfs_mount->get_fs_manager()->getKVStore();
    store->Compact();
}

} // namespace pqfs
