/*
 * KVFSFile.h
 *
 */

#ifndef KV_FS_FILE
#define KV_FS_FILE

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <assert.h>

#include "KVStore.h"

#include "KVFSDirectoryPartition.h"
#include "KVFSMetaData.h"
#include "KVFSObject.h"
#include "fs_change_log.h"  // IWYU: xid_t

#include <unordered_map>
#include <city.h>
#ifdef __SSE4_2__
#include "citycrc.h"
#endif

class KVStore;

namespace pqfs {
class TieringInAgent;
}

class LogFSFile:public LogFSObject {
  protected:
    LogFSFile();

  public:
    LogFSFile(uint64_t fsId,
              const std::string * file_name,
              uint64_t handle,
              KVStore * kvstore,
              pqfs::TieringInAgent* tiering_in_agent,
              mode_t modeInput = 0,
              uint32_t initialSize = DEFAULT_FILE_INITIAL_SIZE,
              uint32_t chunkSize = DEFAULT_FILE_CHUNK_SIZE);

     LogFSFile(const LogFSFile & logFSFile);

     virtual ~ LogFSFile();

    /* Methods overridden from base class */
    Status writeToLog();

    Status readFromLog(std::string & readBuffer);

    bool exists();

    Status create(LogFSFile * parent);

    Status remove(LogFSFile* parent, const char* name);

    Status link(LogFSFile* file, const char* name);

    bool open();

    virtual Status close();     //? may not need?

    Status stat(struct stat *statBuff);

    Status createRoot();
    Status connectDots(LogFSFile* dot, LogFSFile* dotdot);
    uint64_t findHandle(const std::string & name);

    Status read(off_t offset, size_t length, char *readBuffer,
                size_t * actualLen, bool needUpdateAtime);
    Status write(off_t offset, size_t length, const char *writeBuffer,
                 size_t * actualLen, bool update_meta = true);

    Status insertEntry(const uint64_t file_handle,
                       const std::string & file_name);
    Status removeEntry(const uint64_t file_handle,
                       const std::string & file_name);
    Status listDirectory(DirEntryAdder * dir_entry_adder, off_t cur_offset);

    LogFSMetaData& getLogFSMetaData() {
        return meta;
    }
    LogEntryType getType() {
        return type;
    }

    off_t getFileSize() {
        return meta.fileTotalSize;
    }

    uint32_t getInitialSize() {
        return meta.fileInitialSize;
    }

    uint32_t getChunkSize() {
        return meta.fileChunkSize;
    }

    bool isMetaDirty() {
        return metaDirty;
    }

    void setMetaDirty(bool flag) {
        metaDirty = flag;
    }

    void setProtectionFromMode();
    void setTypeFromMode();

    mode_t setModeFromMeta(meta_type_t & type, meta_prot_t & protection);

    void setMode(mode_t new_mode) {
        mode = new_mode;
    }

    void initMeta();

    // Remove the inode and it's data blocks from the KVS.
    // Links (if any) are in place. The inode will be faulted back in
    // when next referenced.
    // TODO(): consider leaving the inode in place, and marking it as faulted out,
    // but it's still necessary to be able to fault in the inode and its data for
    // filesystem takeover.
    Status Evict();

    Status UnEvict();

    Status flush();

    Status access(int mode,
                  const uint32_t userId, const uint32_t groupId, int *pEerror);

    Status chmod(mode_t mode);

    Status chown(const uint32_t userId, const uint32_t groupId);

    Status SetSize(off_t size);

    const off_t get_size() {
        return meta.fileTotalSize;
    }
    const pqfs::xid_t get_xid() {
        return meta.get_xid();
    }

    Status SetAttr(const struct stat *stat);


    bool isDirEmpty();

    Status createRoot(const uint64_t handle);
    size_t get_num_partitions();

  protected:
    void RemoveInodeFromKvs();
    Status ResizeData(off_t new_size, off_t old_size,
            off_t file_chunk_size, off_t file_initial_size);

    void NLinkAdjust(int n);

    time_t UpdateCTime(time_t new_time = time((time_t*)nullptr));
    time_t UpdateMTime(time_t new_time = time((time_t*)nullptr));
    time_t UpdateATime(time_t new_time = time((time_t*)nullptr));

    // get or allocate the specified partition.
    LogFSDirectoryPartition * GetPartition(int i);
    LogFSDirectoryPartition *GetPartition(const std::string & name);
    void ReleasePartition(LogFSDirectoryPartition * partition);

    void HoldData(off_t offset, size_t size);
    void ReleaseData(off_t offset, size_t size);

    KVStore *kvstore;
    pqfs::TieringInAgent* tiering_in_agent;

    LogFSMetaData meta;         //save fixed or static metadata
    mode_t mode;
    bool metaDirty;

    int data_hold_count;
    // Only used by directories.
    std::unique_ptr <
        std::vector < std::unique_ptr < LogFSDirectoryPartition >> >_partitions;

    KVStore *get_kvstore() {
        return kvstore;
    }
};

#endif                          /* KV_FS_FILE */
