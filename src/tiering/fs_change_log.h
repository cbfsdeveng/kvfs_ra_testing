/*
 * API used to access the change log.
 * Used by both writers and consumers.
 * 
 * Change-log may be written to KVS?
 * Else in files (data format?).
 */
#ifndef TIERING_FS_CHANGE_LOG_
#define TIERING_FS_CHANGE_LOG_

#include "change_log.pb.h"
#include <cstdint>

#include <deque>
#include <vector>

namespace pqfs {

/*
 * Each instance manages a single filesystem's change log.
 * This is occasionally persisted to some kind of storage.
 * basic log operations are in-memory only, hence fast.
 * On startup, the in-memory state is read back from storage.
 * We accept, for now, that we may lose updates in the face of unclean shutdown.
 */
typedef int64_t xid_t;
const int64_t XID_MAX = INT64_MAX;

class FsChangeLog {
public:
    //FsChangeLog();  // defaults.
    FsChangeLog(std::string log_dir = "",
            xid_t initial_xid = 0);  // ?Auto increment?

    // setup initial datastructures, read any existing log entries.
    // returns 0 or errno.
    int Init();

    // append(entry, spare?, opaque_info)
    // If 
    // returns the appended entry's xid.
    xid_t Append(proto::ChangeLogEntry& entry);

    // read range. Specified (first..las) entries are copied into "log_entries".
    // Entries will be appended, so "log_entries" should probably be clear when
    // first called.
    // *first_xid is the suggested first (e.g. 0); on return, *first_xid is
    // modified to be the xid of the first entry copied into log_entries.
    // returns 0 on success, errno on failure.
    // State of the log is not changed.
    int Read(
            std::vector<proto::ChangeLogEntry>* log_entries,
            xid_t* first_xid,
            xid_t last_xid) const;

    // trim. Remove entries up to and including xid "trim_to"
    // TODO: is trimming in-core only, or should it also trim persistent
    // copy?
    // TODO: NYI
    // When to do it? How do we know that all interested parties have seen all
    // entries?
    int Trim(xid_t trim_to);

    // Flush current in memory log to persistent storage, appending to existing
    // log.
    int Flush();

    // Read the latest log from persistent storage.
    int Refresh();

    // Returns and increments the next xid. */
    // TODO: locking/atomicity.
    xid_t NextXid() { return next_xid++; }

    xid_t get_first_xid() const {
        return log_deque.empty()? 0 : log_deque.front().xid();
    }
    xid_t get_last_xid() const {
        return log_deque.empty()? 0 : log_deque.back().xid();
    }

    xid_t get_next_xid() const { return next_xid; }  // accessor.
    void set_next_xid(xid_t xid) { next_xid = xid; }

    xid_t get_eviction_next_xid() const { return eviction_next_xid; }  // accessor.
    void set_eviction_next_xid(xid_t xid) { eviction_next_xid = xid; }

    xid_t get_tiered_out_xid() const { return tiered_out_xid; }  // accessor.
    void set_tiered_out_xid(xid_t xid) { tiered_out_xid = xid; }

    xid_t get_size() const { return log_deque.size(); }

private:
    // log_dir is the home of our persistent logs.
    std::string log_dir;

    // TODO: These log pointers need to be persisted with the log,
    // but would be nicer if they were by name or something.
    xid_t next_xid;  // last (latest) log entry.
    xid_t eviction_next_xid;  // next to consider for eviction.
    xid_t tiered_out_xid;     // tiered out changes up to this xid.

    // Using a dequeue for now - always indexed from zero.
    // Note that xid's in log_deque should be increasing, but may not be
    // contiguous, since an allocated xid sometimes can't be used due to errors.
    std::deque<proto::ChangeLogEntry> log_deque;
};

} // namespace pqfs
#endif // TIERING_FS_CHANGE_LOG_
