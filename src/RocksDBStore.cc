/*
 * RocksDBStore.cc
 *
 *  Created on: Oct 15, 2015
 *      Author: Prasanna Ponnada
 */

#include "RocksDBStore.h"
#include <glog/logging.h>
#include "hexdump.h"

bool RocksDBStore::init(const std::string coordinatorLoc,
                        bool hasDispatchedThread)
{
    /* there is no co-ordinator in Level DB, nothing to init */
    return true;
}

bool RocksDBStore::open(const std::string dbName)
{
    rocksdb::Options options;
    options.create_if_missing = true;
    rocksdb::Status status = rocksdb::DB::Open(options, dbName, &db);
    if (status.ok() == false) {
        std::cerr << "Unable to open/create database " << dbName << std::endl;
        std::cerr << status.ToString() << std::endl;
        return false;
    }
    this->db_name = dbName;
    return true;
}

bool RocksDBStore::close()
{
    if (db != NULL) {
        delete db;
        db = NULL;
    }
    return true;
}

bool RocksDBStore::destroy(const std::string dbName)
{
    rocksdb::Options options;
    rocksdb::Status status = rocksdb::DestroyDB(dbName, options);
    if (status.ok() == false) {
        std::cerr << "Unable to destroy database " << dbName << std::endl;
        std::cerr << status.ToString() << std::endl;
        return false;
    }
    return true;
}

bool RocksDBStore::put(const std::string & key, const std::string & value)
{
    VLOG(2) << "put " << key << " size " << value.size();
    rocksdb::Status status = db->Put(rocksdb::WriteOptions(), key, value);
    if (status.ok())
        return true;
    else
        return false;
}

bool RocksDBStore::get(const std::string & key, std::string & value)
{
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), key, &value);
    if (status.ok()) {
        VLOG(2) << "get " << key << " size " << value.size();
        return true;
    }
    VLOG(2) << "get " << key << " failed.";
    return false;
}

bool RocksDBStore::remove(const std::string & key)
{
    rocksdb::Status status = db->Delete(rocksdb::WriteOptions(), key);
    if (status.ok()) {
        VLOG(2) << "remove " << key;
        return true;
    }
    VLOG(2) << "remove " << key << " failed.";
    return false;
}

void RocksDBStore::dump(std::ostream & out)
{
    rocksdb::Iterator * it = db->NewIterator(rocksdb::ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        const rocksdb::Slice & value = it->value();
        out << "key " << it->key().ToString()
            << " size: " << value.size() << std::endl;
        hexdump(value.data(), value.size(), out);
    }
    assert(it->status().ok());  // Check for any errors found during the scan
    delete it;
}

void RocksDBStore::dbstats(std::ostream & out)
{
    rocksdb::Range ranges[1];
    // (0xff hack: wish there was a way to specify last possible key).
    ranges[0] = rocksdb::Range("", "\xff\xff\xff\xff");
    uint64_t sizes[1];
    db->GetApproximateSizes(ranges, 1, sizes);
    out << "\nApproximate database size: " << sizes[0];
    //  "rocksdb.num-files-at-level<N>" - return the number of files at level <N>,
    //     where <N> is an ASCII representation of a level number (e.g. "0").
    //  "rocksdb.stats" - returns a multi-line string that describes statistics
    //     about the internal operation of the DB.
    //  "rocksdb.sstables" - returns a multi-line string that describes all
    //     of the sstables that make up the db contents.
    //  "rocksdb.approximate-memory-usage" - returns the approximate number of
    //     bytes of memory in use by the DB.
    for (auto& prop : {
#if 0  // these are redundant with stats.
            "rocksdb.num-files-at-level0",
            "rocksdb.num-files-at-level1",
            "rocksdb.num-files-at-level2",
            "rocksdb.num-files-at-level3",
            "rocksdb.num-files-at-level4",
            "rocksdb.num-files-at-level5",
#endif
            "rocksdb.stats",
            "rocksdb.sstables",
            "rocksdb.approximate-memory-usage"
            }
        )
    {
        std::string val;
        db->GetProperty(prop, &val);
        out << std::endl << "prop: " << prop << ": " << val;
    }
}

void RocksDBStore::Compact() {
    // (0xff hack: wish there was a way to specify last possible key).
    rocksdb::Slice range_begin("");
    rocksdb::Slice range_end("\xff\xff\xff\xff");
    VLOG(1) << "CompactRange begin";
    db->CompactRange(&range_begin, &range_end);
    VLOG(1) << "CompactRange end";
}

RocksDBStore *RocksDBStore::Snapshot()
{
    if (is_snapshot) {
        LOG(ERROR) << "RocksDBStore::Snapshot of snapshot";
        return nullptr;
    }
    auto *snapshot = new RocksDBStore();
    snapshot->is_snapshot = true;
    snapshot->read_options = new rocksdb::ReadOptions();
    snapshot->read_options->snapshot = this->db->GetSnapshot();
    snapshot->db = this->db;    // Not owned - delete snapshot before base db.
    snapshot->db_name = this->db_name + ".SNAP";
    return snapshot;
}

RocksDBStore::~RocksDBStore()
{
    if (is_snapshot &&
        read_options != nullptr && read_options->snapshot != nullptr) {
        db->ReleaseSnapshot(read_options->snapshot);
        read_options->snapshot = nullptr;
        delete read_options;
        read_options = nullptr;
        is_snapshot = false;
    }
}
