/*
 * FileSystemManager.h
 *
 *  Created on: Oct 15, 2015
 *      Author: Prasanna Ponnada
 */

#ifndef KVS_FILESYSTEM_MANAGER
#define KVS_FILESYSTEM_MANAGER

#include <city.h>
#ifdef __SSE4_2__
#include "citycrc.h"
#endif

#include <fuse_lowlevel.h>  // for FUSE_ROOT_ID.
#include "KVStore.h"
#include "KVFSMetaData.h"
#include "KVFSFile.h"
#include "tiering/fs_change_log.h"

#include "Status.h"
#include "Atomic.h"
#include <boost/scoped_ptr.hpp>

namespace pqfs {
class TieringInAgent;
}

class FileSystemManager {
public:
    explicit
	FileSystemManager(KVStore* kvStore);
    
    void setTieringInAgent(pqfs::TieringInAgent* new_tiering_in_agent) {
        tiering_in_agent = new_tiering_in_agent;
    }

    bool mountFileSystem(
                const std::string& fsName,
                pqfs::FsChangeLog* change_log = nullptr);  // not owned

    bool
	umountFileSystem();

    FileSystemManager* MountSnapshot();
    void UnmountSnapshot();

    Status    lookupHandle(uint64_t fsId,
                                const uint64_t parent_handle,
                                const std::string& name,
                                uint64_t* handle);

    KVStore* getKVStore() {
    	return kvstore;
    }

    Status createRoot(uint64_t fsId, mode_t mode);

    Status addDirectory(uint64_t fsId,
                                const uint64_t parent_handle,
                                const std::string& dir_name,
                                mode_t mode,
                                uint64_t& handle);


    Status readFile(uint64_t fsId,
                             const uint64_t handle,
                             uint64_t offset,
                             uint64_t length,
                             char* readBuffer,
                             uint64_t* actualLen,
                             bool needUpdateAtime);

   Status statFile(uint64_t fsId,
                                    const uint64_t handle,
                                    struct stat *statBuff);

   Status getPathInfo(uint64_t fsId, uint64_t handle,
           uint64_t* parent_handle, std::string* file_name);

   Status openFile(uint64_t fsId,
                              const uint64_t handle,
                              mode_t mode);

    // Remove this inode and it's data from the key-value store.
    // Future references will fault it in from the proxy or the cloud.
    Status Evict(uint64_t fs_id, uint64_t handle);

   Status releaseFile(uint64_t fsId, const uint64_t handle);
    inline uint64_t
    getNextHandleId() {
        return handleId.operator ++();
    }

    void SetNextInode(uint64_t id) {
        handleId = id;
    }

    inline uint64_t
    GetCurrentInode() {
        return handleId;
    }

    static const uint64_t ROOT_HANDLE = FUSE_ROOT_ID;


private:
    KVStore* kvstore;
    pqfs::TieringInAgent* tiering_in_agent;
    pqfs::FsChangeLog* change_log = nullptr;  // Not owned.

    Atomic<uint64_t> handleId;
};

#endif  // KVS_FILESYSTEM_MANAGER
