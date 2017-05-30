/*
 * KVFSObject.h
 *
 *  Created on: Oct 15, 2015
 *      Author: Prasanna Ponnada
 */

#ifndef KVFS_OBJECT
#define KVFS_OBJECT

#include "Status.h"
#include "KVStore.h"
#include "KVFSMetaData.h"


class LogFSObject {
private:
    LogFSObject();

public:

//    LogFSObject(uint64_t fsId, const void* parent, ObjectNameLength parentLength,
//               const void* name, ObjectNameLength nameLength, uint64_t handle,
//               LogEntryType type);

    LogFSObject(uint64_t fsId,
            uint64_t handle, LogEntryType type,
            const std::string* name);

    virtual ~LogFSObject();
    
    void setHandle(uint64_t handle);

    void setParentHandle(uint64_t handle);

    uint64_t getFSId() {
            return fsId;
    }

#ifdef NOT_USED
    std::string getParentDirName() {
         //return std::string((const char*)parent);
         return parentString;
    }
#endif

    uint64_t getHandle() {
        return handle;
    }

    LogEntryType getType() {
        return type;
    }
    

    // This returns the nameString, as set by one of our constructors.
    const std::string& getName() {
        //return std::string((const char*)name);
        return nameString;
    }

    // This returns the nameString, as set by one of our constructors.
    void setName(std::string& name) {
        if (!nameString.empty()) {
            LOG(WARNING) << "setName " << name << " not empty: " << nameString;
        }
        nameString = name;
    }

    protected:
    /* Every object belongs to a file system with a 64bit UUID */
    uint64_t fsId;
    uint64_t handle;
    LogEntryType type;

    /// The tuple <fsId, parent, name> forms the Key.
    /// Except for root dir "/", every object in the filesystem has a parent
    /// Pointer to the binary string key.
//    const void* parent;

    /// Length of the binary string key in bytes.
//    ObjectNameLength parentLength;

    /* Every object (dir/file/snapshot/clone/digest/..) has a name in the FS */
    /* Some objects (Eg: dir, file, snapshot, clone) have 1 or more indexes.
     * Those subclasses add their own private members for those indexes.
     */
//    const void* name;
    /// Length of the binary string key in bytes.
//    ObjectNameLength nameLength;

    std::string parentString;
    std::string nameString;

};

#endif /* LOGFS_OBJECT */
