/*
 * LevelDBStore.cc
 *
 *  Created on: Oct 15, 2015
 *      Author: Prasanna Ponnada
 */

#include "LevelDBStore.h"
#include <glog/logging.h>
#include "hexdump.h"

bool LevelDBStore::init(const std::string coordinatorLoc,
                        bool hasDispatchedThread)
{
    /* there is no co-ordinator in Level DB, nothing to init */
    return true;
}

bool LevelDBStore::open(const std::string dbName)
{
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, dbName, &db);
    if (status.ok() == false) {
        std::cerr << "Unable to open/create database " << dbName << std::endl;
        std::cerr << status.ToString() << std::endl;
        return false;
    }
    this->db_name = dbName;
    return true;
}

bool LevelDBStore::close()
{
    if (db != NULL) {
        delete db;
        db = NULL;
    }
    return true;
}

bool LevelDBStore::destroy(const std::string dbName)
{
    leveldb::Options options;
    leveldb::Status status = leveldb::DestroyDB(dbName, options);
    if (status.ok() == false) {
        std::cerr << "Unable to destroy database " << dbName << std::endl;
        std::cerr << status.ToString() << std::endl;
        return false;
    }
    return true;
}

bool LevelDBStore::put(const std::string & key, const std::string & value)
{
    bool ret = true;
    leveldb::Status status = db->Put(leveldb::WriteOptions(), key, value);
    if (!status.ok())
        ret = false;
    VLOG(2) << "db:put " << key << " size " << value.size() << " ret " << ret;
    return ret;
}

bool LevelDBStore::get(const std::string & key, std::string & value)
{
    leveldb::Status status = db->Get(leveldb::ReadOptions(), key, &value);
    if (status.ok()) {
        VLOG(2) << "db:get " << key << " size " << value.size();
        return true;
    }
    VLOG(2) << "db:get " << key << " failed.";
    return false;
}

bool LevelDBStore::remove(const std::string & key)
{
    leveldb::Status status = db->Delete(leveldb::WriteOptions(), key);
    if (status.ok()) {
        VLOG(2) << "db:remove " << key;
        return true;
    }
    VLOG(2) << "db:remove " << key << " failed.";
    return false;
}

void LevelDBStore::dump(std::ostream & out)
{
    leveldb::Iterator * it = db->NewIterator(leveldb::ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        const leveldb::Slice & value = it->value();
        out << "key " << it->key().ToString()
            << " size: " << value.size() << std::endl;
        hexdump(value.data(), value.size(), out);
    }
    assert(it->status().ok());  // Check for any errors found during the scan
    delete it;
}

void LevelDBStore::dbstats(std::ostream & out)
{
    leveldb::Range ranges[1];
    // (0xff hack: wish there was a way to specify last possible key).
    ranges[0] = leveldb::Range("", "\xff\xff\xff\xff");
    uint64_t sizes[1];
    db->GetApproximateSizes(ranges, 1, sizes);
    out << "\nApproximate database size: " << sizes[0];
    //  "leveldb.num-files-at-level<N>" - return the number of files at level <N>,
    //     where <N> is an ASCII representation of a level number (e.g. "0").
    //  "leveldb.stats" - returns a multi-line string that describes statistics
    //     about the internal operation of the DB.
    //  "leveldb.sstables" - returns a multi-line string that describes all
    //     of the sstables that make up the db contents.
    //  "leveldb.approximate-memory-usage" - returns the approximate number of
    //     bytes of memory in use by the DB.
    for (auto& prop : {
#if 0  // these are redundant with stats.
            "leveldb.num-files-at-level0",
            "leveldb.num-files-at-level1",
            "leveldb.num-files-at-level2",
            "leveldb.num-files-at-level3",
            "leveldb.num-files-at-level4",
            "leveldb.num-files-at-level5",
#endif
            "leveldb.stats",
            "leveldb.sstables",
            "leveldb.approximate-memory-usage"
            }
        )
    {
        std::string val;
        db->GetProperty(prop, &val);
        out << std::endl << "prop: " << prop << ": " << val;
    }
}

void LevelDBStore::Compact() {
    // (0xff hack: wish there was a way to specify last possible key).
    leveldb::Slice range_begin("");
    leveldb::Slice range_end("\xff\xff\xff\xff");
    VLOG(1) << "db:CompactRange begin";
    db->CompactRange(&range_begin, &range_end);
    VLOG(1) << "db:CompactRange end";
}

LevelDBStore *LevelDBStore::Snapshot()
{
    if (is_snapshot) {
        LOG(FATAL) << "LevelDBStore::Snapshot of snapshot";
        return nullptr;
    }
    auto *snapshot = new LevelDBStore();
    snapshot->is_snapshot = true;
    snapshot->read_options = new leveldb::ReadOptions();
    snapshot->read_options->snapshot = this->db->GetSnapshot();
    snapshot->db = this->db;    // Not owned - delete snapshot before base db.
    snapshot->db_name = this->db_name + ".SNAP";
    return snapshot;
}

LevelDBStore::~LevelDBStore()
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
