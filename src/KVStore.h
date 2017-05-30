/*
 * KVStore.h
 *
 *  Created on: Oct 12, 2015
 *      Author: Prasanna Ponnada
 */

#include <iostream>
#ifndef KVSTORE_H
#define KVSTORE_H

class KVStore {
    public:
    KVStore() {};
    //
    virtual bool init(const std::string locator, bool hasDispatchedThread) = 0;
    //
    virtual bool open(const std::string dbname) = 0;
    //
    virtual bool put(const std::string& key, const std::string& value) = 0;
    //
    virtual bool get(const std::string& key, std::string& value) = 0;
    //
    virtual bool remove(const std::string& key) = 0;
    //
    virtual bool close() = 0;
    //
    virtual bool destroy(const std::string dbname) = 0;
    // Dump the store: for debugging purposes only.
    virtual void dump(std::ostream& out) = 0;
    //
    virtual void dbstats(std::ostream& out) = 0;

    // Compact the database (remove dead space synchronously instead of in bg).
    virtual void Compact() = 0;

    //
    virtual ~KVStore() {};
    // Create a snapshot of the current (non-snapshot) KVStore.
    // Caller should delete when done (which should trigger release of
    // snapshot in underlying kv-store).
    virtual KVStore* Snapshot() = 0;

    //
    virtual bool IsSnapshot() = 0;
};

#endif /* KVSTORE_H */
