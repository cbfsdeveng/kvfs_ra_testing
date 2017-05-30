/*
 * LevelDBStore.h
 *
 *  Created on: Oct 12, 2015
 *      Author: Prasanna Ponnada
 */

#ifndef LEVELDBSTORE_H
#define LEVELDBSTORE_H

#include <iostream>
#include "KVStore.h"
#include "leveldb/db.h"

class LevelDBStore : public KVStore {
public:
	LevelDBStore() :
            db(nullptr),
            is_snapshot(false),
            read_options(nullptr) {};

        ~LevelDBStore();

	virtual bool
	init(const std::string dbAbsolutePath, bool hasDispatchedThread) override;

	virtual bool open(const std::string dbname) override;

	virtual bool put(const std::string& key, const std::string& value) override;

	virtual bool get(const std::string&key, std::string& value) override;

	virtual bool remove(const std::string& key) override;
	virtual bool close() override;
	virtual bool destroy(const std::string dbname) override;
        virtual void dump(std::ostream& out);
        virtual void dbstats(std::ostream& out);

	static void setDBAbsolutePath(std::string dbAbsolutePath);
        // Create a snapshot of the current (non-snapshot) KVStore.
        LevelDBStore* Snapshot();
        bool IsSnapshot() { return is_snapshot; }

        void Compact();

private:    
    leveldb::DB* db;  // ugh - not owned unless snapshot.
    std::string db_name;  // name passed to open.
    // Used when reading from a snapshot.
    bool is_snapshot;
    leveldb::ReadOptions* read_options;
};

#endif /* LEVELDBSTORE_H */
