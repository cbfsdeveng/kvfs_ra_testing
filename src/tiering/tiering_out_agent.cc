#include "tiering_out_agent.h"
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

void Compress(std::vector<proto::ChangeLogEntry>* log_entries);

int TieringOutAgent::SetOptions(std::string options) {
    // TODO:
    return 0;
}

int TieringOutAgent::RequestStop() {
    // NEEDSWORK: synchronize, notify.
    stop_requested = true;
    return 0;
}

void TieringOutAgent::Join() {
    assert(run_thread);
    run_thread->join();
    delete run_thread;
    run_thread = nullptr;
}

// block calling thread until transactions up to xid have been flushed.
xid_t TieringOutAgent::Wait(xid_t flushed_to_xid, int32_t millis) {
    // TODO: Locking & synchronization.
    while (fs_change_log->get_tiered_out_xid() < flushed_to_xid) {
        if (millis <= 0)
            break;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        millis -= 1000;
    }
    return fs_change_log->get_tiered_out_xid();  // Caller can tell whether we got to flushed_to_xid.
}

#define MAX_XID_PER_SEGMENT  1000

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

void TieringOutAgent::RunInternal() {
        /*
        RunInternal() ...
        Loop:
            Wait for {change count | time | sync request}:
            Snapshot filesystem (part of filesystem api).
            (for first demo, snapshot may just use current mount)
            Ask snapshot for it's version number.
            Read log[current..snapshot]
            compress(current_snapshot_log)
            generate paths for items in log
            (get path via ioctl that gives base obj for inode? or read kvs)
            sort on path (for grouping).
            estimate sizes
            group for tarballs
            for each tarball:
                // describing how to do w tar command here.
                // Needswork: Eventually write directly via libtar.
***             create a temp dir to hold future contents,
***             for each inode in tarball:
***               lookup path
***               copy file contents and metadata to that path under temp dir.
***               // libtar: directories? add bsd-tar dirblocks?
***             create manifest/index of tarball (tail-written? append to tarball?)
***             // write (gnu incremental-ish) tarball (segment);
***             write index segment
            write new index using new index segments and existing index.
            (or just write new index based on in-memory index, read in from
            files).
        */

    do {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        VLOG(4) << "RunInternal woke up";
        if (snapshot_requested) {
            Snapshot();
            snapshot_requested = false;
        }
    } while (!stop_requested);
}

void TieringOutAgent::Snapshot() {
    xid_t last_xid = -1;
    xid_t snapshot_end_xid = fs_change_log->get_last_xid();
    do {
        std::vector<proto::ChangeLogEntry> log_entries;
        xid_t first_xid = fs_change_log->get_tiered_out_xid() + 1;
        // TODO: smarter way to limit segment sizes.
        // Note that xid's in fs_change_log are strictly increasing, but not
        // necessarily contiguous.
        last_xid = std::min(first_xid + MAX_XID_PER_SEGMENT, snapshot_end_xid);
        fs_change_log->Read(&log_entries, &first_xid, last_xid);
        LOG(INFO) << "Snapshot: snapshot_end " << snapshot_end_xid
                << " first_xid " << first_xid
                << " last " << last_xid
                << " read " << log_entries.size() << " entries";
        // remove moot entries and merging overlapping ones.
        Compress(&log_entries);
        VLOG(1) << "Snapshot: " << log_entries.size() << "log ents:";
        if (VLOG_IS_ON(3)) {
            for (size_t i = 0; i < log_entries.size(); i++) {
                std::string log_ent_as_string;
                TextFormat::PrintToString(log_entries[i],
                        &log_ent_as_string);
                VLOG(3) << "Snapshot: " << i << " "
                    << log_ent_as_string;
            }
        }
        // Get the list of unique inodes in this batch of changes.
        std::vector<uint64_t> inode_nums;
        GetInodes(&inode_nums, /*const&*/log_entries);
        assert(log_entries.back().has_xid());
        last_xid = log_entries.back().xid();

        FileSystemManager* snapshot_fs_manager =
                kvfs_mount->get_fs_manager()->MountSnapshot();
        SnapshotPersister snapshot_persister(
                        snapshot_fs_manager,  // owned by persister.
                        // Owned by persister.
                        cloud_proxy);
        // This will synchronously create a tarball(s) for these inodes and (?)
        // push it to the cloud on the current thread.
        // TODO: named snapshots?
        snapshot_persister.Snapshot(
                segment_factory,
                index_manager,
                &inode_nums,
                first_xid,
                last_xid);
        snapshot_persister.Close();
        VLOG(1) << "Snapshot: update tiered_out_xid from "
                << fs_change_log->get_tiered_out_xid() << " to " << last_xid;
        fs_change_log->set_tiered_out_xid(last_xid);
    } while (fs_change_log->get_tiered_out_xid() < snapshot_end_xid);
}

// This should be in another class (LogCompressor).
// Global func since doesn't need to access any tiering-agent state.
void Compress(
        std::vector<proto::ChangeLogEntry>* log_entries) {
    // NEEDSWORK: Put MK's code here, from pq_pcsnslogg/compress.cc.
    // (does his code also merge metadata updates?)
    LOG(WARNING) << "TieringOutAgent::Compress not yet implemented.";
}

// helper.
void GetInodes(
        std::vector<uint64_t>* inode_nums,
        const std::vector<proto::ChangeLogEntry> log_entries) {
    std::set<uint64_t> unique_inodes;
    for (size_t i = 0; i < log_entries.size(); i++) {
        const proto::ChangeLogEntry& log_ent = log_entries[i];
        if (log_ent.has_inode_num()) {
            unique_inodes.insert(log_ent.inode_num());
        } else {
            std::string log_ent_as_string;
            TextFormat::PrintToString(log_ent, &log_ent_as_string);
            LOG(WARNING) << "GetInodes: entry !has_inode: "
                <<  log_ent_as_string;
        }
    }
    for (auto inode_num : unique_inodes) {
        inode_nums->push_back(inode_num);
    }
    LOG(INFO) << "GetInodes: " << inode_nums->size() << " inodes";
}

int TieringOutAgent::Start() {
    if (run_thread != nullptr) {
        return EINVAL;
    }
    // starts running immediately.
    stop_requested = false;
    run_thread = new std::thread(&TieringOutAgent::RunInternal, this);
    return 0;
}

// (event_string == "/DUMP")
void TieringOutAgent::DumpKVStore(std::ostringstream& result) {
    KVStore* store = kvfs_mount->get_fs_manager()->getKVStore();
    if (store != nullptr) {
        store->dump(result);
    }
}

// (event_string == "/DBSTATS") {
void TieringOutAgent::DBStats(std::ostringstream& result) {
    KVStore* store = kvfs_mount->get_fs_manager()->getKVStore();
    if (store != nullptr) {
        store->dbstats(result);
    }
}

} // namespace pqfs
