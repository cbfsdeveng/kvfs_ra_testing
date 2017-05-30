/*
 * FileSystemManager.cc
 *
 *  Created on: Oct 15, 2015
 *      Author: Prasanna Ponnada
 *
 * This is largely deprecated as we move towards using inodes
 * (see kvfs_mount.*).
 */
#include "FileSystemManager.h"
#include "KVFSFile.h"

#include "Cycles.h"
#include "WallTime.h"
#include <glog/logging.h>

FileSystemManager::FileSystemManager(KVStore* kvStore)
    : kvstore(kvStore),
      tiering_in_agent(nullptr),
    handleId(ROOT_HANDLE)
{
}

bool
FileSystemManager::mountFileSystem(
        const std::string& fsName,
        pqfs::FsChangeLog* change_log) { // = nullptr
    bool ret = true;
    (void)fsName;  // key value stopre now opened elsewhere.
    this->change_log = change_log;
    if (this->change_log != nullptr) {
        this->change_log->Init();
    }
    return ret;
}

bool
FileSystemManager::umountFileSystem() {

    return this->getKVStore()->close();
}

FileSystemManager*
FileSystemManager::MountSnapshot() {
    auto* store_snapshot = kvstore->Snapshot();
    // we should never access an evicted file in the snapshot, so shouldn't
    // ever need a TieringInAgent.
    auto* snapshot_filesystem = new FileSystemManager(store_snapshot);
    // Note that we don't fill in the snapshot FS's change_log, since it is pure
    // read-only and shouldn't see any updates.
    return snapshot_filesystem;
}

void
FileSystemManager::UnmountSnapshot() {
    // This method is only called on snapshot-filesystems, which own their
    // kvstore.
    // TODO: I don't love having the kvstore owned in some cases and not others.
    delete kvstore;
    kvstore = nullptr;
}

Status
FileSystemManager::createRoot(uint64_t fsId,
                                mode_t mode)
{
    LogFSFile root_dir(fsId, nullptr, ROOT_HANDLE, kvstore, tiering_in_agent,
            S_IFDIR | mode);
    Status status = root_dir.createRoot();
    // TODO: does the root need a log entry?
    if (status == Status::STATUS_OK) {
         pqfs::proto::ChangeLogEntry entry;
         entry.set_operation(pqfs::proto::ChangeLogEntry::MKDIR);
         entry.set_fs_id(fsId);
         entry.set_inode_num(ROOT_HANDLE);
         entry.set_parent_inode_num(ROOT_HANDLE);
         change_log->Append(entry);
    }
    return status;
}

/*
void (*mkdir) (fuse_req_t req, fuse_ino_t parent, const char *name,
           mode_t mode);
*/
Status
FileSystemManager::addDirectory(uint64_t fsId,
                                const uint64_t parent_handle,
                                const std::string& dir_name,
                                mode_t mode,
                                uint64_t& handle)
{
    uint64_t dir_handle = this->getNextHandleId(); //if multiple threads run concurrently?

    LogFSFile parent_dir(fsId, nullptr, parent_handle,
        kvstore, tiering_in_agent, S_IFDIR | mode);
    LogFSFile dir(fsId, &dir_name, dir_handle,
            kvstore, tiering_in_agent, S_IFDIR | mode);
    dir.getLogFSMetaData().addPathInfo(
               parent_handle, dir_name);
    Status status = dir.create(&parent_dir);
    if (status == Status::STATUS_OK) {
         pqfs::proto::ChangeLogEntry entry;
         entry.set_operation(pqfs::proto::ChangeLogEntry::MKDIR);
         entry.set_fs_id(fsId);
         entry.set_inode_num(dir_handle);
         entry.set_parent_inode_num(parent_handle);
         change_log->Append(entry);
    }
    handle = dir_handle;
    return status;
}

// TODO: Still used to read files for tarballs (tiering/segment.cc)
Status
FileSystemManager::readFile(uint64_t fsId,
                             const uint64_t handle,
                             uint64_t offset,
                             uint64_t length,
                             char* readBuffer,
                             uint64_t* actualLen,
                             bool needUpdateAtime) {

    Status status = Status::STATUS_OK;
    LogFSFile logFSFile(fsId, nullptr, handle, kvstore, tiering_in_agent);
    if(logFSFile.open()) {
            status = logFSFile.read(offset, length, readBuffer,
                                    actualLen, needUpdateAtime);
    }
    else {
        *actualLen = 0;
        status = Status::STATUS_OBJECT_DOESNT_EXIST;
    }

    return status;  //release fileLock
}

/*
 void (*getattr) (fuse_req_t req, fuse_ino_t ino,  struct fuse_file_info *fi);
*/
Status FileSystemManager::statFile(uint64_t fsId,
                                    const uint64_t handle,
                                    struct stat *statBuff) {

    Status status = Status::STATUS_OK;

    //no cache first
    LogFSFile logFSFile(fsId, nullptr, handle, kvstore, tiering_in_agent);

    if(logFSFile.stat(statBuff)) {
        status = Status::STATUS_OBJECT_DOESNT_EXIST;
    }

    /*Temporarily add for the directory,
         need be deleted when the directory's metadata has been changed*/
    // NEEDSWORK: Overwrite some fields with fake values?
    // Not sure why.
    if(!S_ISREG(statBuff->st_mode)) {

        //setTypeFromMode();         //comes from mode_t
        //setProtectionFromMode(); //comes from mode_t
        statBuff->st_mode = S_IFDIR | 0777;  // TODO: ??? wtf

        /*need change later*/
        statBuff->st_uid = getuid();  //user ID of owner
        statBuff->st_gid = getgid(); //group ID of the owner

        statBuff->st_dev = fsId;    //backing device ID
        statBuff->st_rdev = 0; //device ID(if special file)

        //fileTotalSize;
        //fileInitialSize;
        //fileChunkSize;

        statBuff->st_nlink  = 2; //link count

        //if need higher resolution, then we can extend WallTime use
        //gettimeofday() to get higher resolution
        /* last status change time */
        statBuff->st_ctime = (uint32_t)
        WallTime::secondsTimestampToUnix(WallTime::secondsTimestamp());
        statBuff->st_atime = statBuff->st_ctime;      /* last access time */
        statBuff->st_mtime = statBuff->st_ctime;      /* last modification time */
    }

    return status;
}

Status FileSystemManager::getPathInfo(uint64_t fsId,
                                    uint64_t handle,
                                    uint64_t* parent_handle,
                                    std::string* file_name) {
    Status status = Status::STATUS_OBJECT_DOESNT_EXIST;

    LogFSFile logFSFile(fsId, nullptr, handle, kvstore, tiering_in_agent);
    if(logFSFile.open()) {
#ifdef PATH_INFO_IN_META
        // *parent_handle = logFSFile.getParentHandleFromMeta();
        // *file_name = logFSFile.getNameFromMeta();
        logFSFile.getLogFSMetaData().getPathInfo(
               parent_handle, file_name);
#else
        *parent_handle = 0;  // Invalid.
        *file_name = logFSFile.getName();  // uninitialized?
#endif
        status = Status::STATUS_OK;
        VLOG(1) << "getPathInfo: parent_handle " << *parent_handle
            << " name " << file_name;
    }
    return status;
}

/*
void (*open) (fuse_req_t req, fuse_ino_t ino,  struct fuse_file_info *fi);
*/
Status
FileSystemManager::openFile(uint64_t fsId,
                              const uint64_t handle,
                              mode_t mode) {

    Status status = Status::STATUS_OK;
/*
    std::string fullName = format("%lu", handle);
    Key fullNameKey(fsId, fullName.c_str(),
                                  downCast<KeyLength>(fullName.length()));
    FileLock lock(*this, fullNameKey);
*/
    //no cache at first
    LogFSFile logFSFile(fsId, nullptr, handle, kvstore, tiering_in_agent);
    if(!logFSFile.open()) {

        status = Status::STATUS_OBJECT_DOESNT_EXIST;
    }

    return status;
}

Status
FileSystemManager::Evict(uint64_t fs_id, uint64_t handle) {
    LogFSFile logFSFile(fs_id, nullptr, handle, kvstore, tiering_in_agent);
    return logFSFile.Evict();
}

/*
 	void (*release) (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);
*/
Status FileSystemManager::releaseFile(uint64_t fsId,
                                        const uint64_t handle)
{
    Status status = Status::STATUS_OK;

    //At first no  cache, then no need to flush
    return status;
}

