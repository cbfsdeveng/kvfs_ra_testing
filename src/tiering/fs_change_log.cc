/*
 * Implements a change log: a (numbered) sequence of filesystem change
 * operations (ChangeLogEntry instances - from protocol buffer).
 * Used by both writers and consumers.
 * 
 * Change-log may be written to KVS?
 * Else in files (data format?).
 * If in files, use protocol buffer text format.
 */

#include "fs_change_log.h"
#include <dirent.h>
#include <fstream>
#include <google/protobuf/text_format.h>
#include <glog/logging.h>

using namespace google::protobuf;


namespace pqfs {

/*
 * Each instance manages a single filesystem's change log.
 * This is occasionally persisted to some kind of storage.
 * basic log operations are in-memory only, hence fast.
 * On startup, the in-memory state is read back from storage.
 * We accept, for now, that we may lose updates in the face of unclean shutdown.
 */
FsChangeLog::FsChangeLog(std::string log_dir, xid_t initial_xid) :
        log_dir(log_dir), next_xid(initial_xid),
        eviction_next_xid(initial_xid),
        tiered_out_xid(initial_xid) {
}

// setup initial datastructures, read any existing log entries.
// returns 0 or errno.
int FsChangeLog::Init()
{
    LOG(INFO) << "FsChangeLog::Init";
    return 0;
}

// Returns the xid just added, or invalid value on error.
xid_t
FsChangeLog::Append(proto::ChangeLogEntry& entry) {
    if (!entry.has_xid()) {
        entry.set_xid(NextXid());
    }
    {
        // std::string entry_as_string;
        // TextFormat::PrintToString(entry, &entry_as_string); 
        VLOG(2) << "FsChangeLog::Append " << entry.ShortDebugString();
    }
    log_deque.push_back(entry);
    return entry.xid();;
}

int FsChangeLog::Read(
        std::vector<proto::ChangeLogEntry>* log_entries,
        xid_t* first_xidp,
        xid_t last_xid) const {
    const xid_t zeroth_xid = next_xid - log_deque.size();
    int64_t first_index = *first_xidp - zeroth_xid;
    if (first_index < 0) {  // first may be before start of queue.
        first_index = 0;
    }
    int64_t last_index = last_xid - zeroth_xid;
    // assert((int)log_deque.>size() >= 0)
    if (last_index >= (int)log_deque.size())
        last_index = log_deque.size() - 1;  // may be negative.
    VLOG(2) << "FsChangeLog::Read: first_index " << first_index
        << " last_index " << last_index
        << " size " << log_deque.size();
    *first_xidp = zeroth_xid + first_index;
    // Needswork: Efficiency? We're copying (should we be?), and not using an
    // iterator.
    for (int i = first_index; i <= last_index; i++) {
        log_entries->push_back(log_deque.at(i));
    }
    return 0;
}

// HACK!! NEEDSWORK:
// Remember name of most recently written log file to test Refresh.
std::string last_log_file;

// Flush current in memory log to persistent storage,
// Since the file name is based on current xid range being flushed, we expect to
// be createing a new file.
// TODO: Should we be appending new entries and renaming, since often, most of
// the in-core log is already on disk?
int FsChangeLog::Flush() {
    std::ostringstream log_file;
    log_file << log_dir << "/"
            << log_deque.front().xid() << "-"
            << log_deque.back().xid() 
            << ".pqfslog";
    std::ofstream log_ofs(log_file.str(),
            std::ios::out
            | std::ios::trunc
            // | std::ios::app
            | std::ios::binary);
    if( !log_ofs.is_open() ) {
        LOG(FATAL) << "Flush: can't open " << log_file.str();
    }
    last_log_file = log_file.str();
    int num_log_ents = 0;
    for (auto& log_ent : log_deque) {
        VLOG(3) << "FsChangeLog::Flush " << log_ent.ShortDebugString();
        // NEEDSWORK: probably at least one or two extra copies happening
        std::string serialized;
        log_ent.SerializeToString(&serialized);
        size_t size = serialized.size();
        // NEEDSWORK: endian-ness?
        log_ofs.write((char*)&size, sizeof size);
        log_ofs.write(serialized.c_str(), size);
        //log_ent.SerializeToOstream(log_ofs);
        num_log_ents++;
    }
    VLOG(2) << "FsChangeLog::Flush num_log_ents " << num_log_ents;
    return 0;
}

// Read the latest log from persistent storage.
// For now, this is only intended to be used when the log is initialized,
// hence our log_deque should be empty, which simplifies insertions. If we ever
// need to merge from persistent into memory, this will be trickier (deque clear
// and insert I guess? or use a vector instead of deque).
int FsChangeLog::Refresh() {
    // TODO: Use dirent.h / opendir / readdir / closedir to scan for log files
    // to read. Here we're just using static global last_log_file (hack).
    std::ifstream log_ifs(last_log_file, std::ios::in | std::ios::binary);
    if( !log_ifs.is_open() ) {
        LOG(FATAL) << "Refresh: can't open " << last_log_file;
    }
    if (log_deque.size() != 0) {
        LOG(WARNING) << "FsChangeLog::Refresh() log_deque size " <<
            log_deque.size() << " != 0";
    }
    int num_entries_read = 0;
    for ( ; ; ) {
        size_t entry_size;
        char entry_bytes[1024];
        log_ifs.read((char*)&entry_size, sizeof entry_size);
        if (!log_ifs) {
            if (log_ifs.eof()) {
                break;
            }
            LOG(FATAL) << "FsChangeLog::Refresh log_ifs read error";
        }
        if (entry_size >= 1024) {
            LOG(FATAL) << "FsChangeLog::Refresh entry too large "
                << entry_size;
        }
        log_ifs.read(entry_bytes, entry_size);
        if (!log_ifs) {
            // NEEDSWORK: tolerate incomplete log files.
            LOG(FATAL) << "!log_ifs after read " << last_log_file;
        }
        std::string log_entry_string(entry_bytes, entry_size);
        std::istringstream log_entry_stream(log_entry_string);
        proto::ChangeLogEntry log_entry;
        if (!log_entry.ParseFromIstream(&log_entry_stream)) {
            LOG(ERROR) << "FsChangeLog::Refresh: parse failed";
        }
        VLOG(2) << "Refresh: entry " << log_entry.ShortDebugString();
        // Note that we don't use Append() here because that would overwrite the
        // xid.
        if (log_entry.has_xid()) {
            log_deque.push_back(log_entry);
            next_xid = log_entry.xid() + 1;
        } else {
            LOG(WARNING) << "FsChangeLog::Refresh: no xid " <<
                log_entry.ShortDebugString();
            continue;
        }
        num_entries_read++;
    }
    return num_entries_read;
}

} // namespace pqfs
