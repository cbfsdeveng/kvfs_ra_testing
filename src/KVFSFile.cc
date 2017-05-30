
#include <algorithm>    // std::min
#include <assert.h>
#include <stdio.h>
#include <string>
#include <string.h>

#include "Cycles.h"
#include "expect.h"

#include "FileSystemManager.h"
#include "KVFSDirectoryPartition.h"
#include "KVFSFile.h"
#include "tiering_in_agent.h"
#include "tiered_in_file.h"


std::string hex(off_t off)
{
    std::ostringstream ss;
    ss << "0x" << std::hex << off;
    return ss.str();
}

LogFSFile::LogFSFile(uint64_t fsId,
                     const std::string* file_name,
                     const uint64_t handle,
                     KVStore* kvstore,
                     pqfs::TieringInAgent* tiering_in_agent,
                     mode_t modeInput,
                     uint32_t initialSize,
                     uint32_t chunkSize):
                     LogFSObject(fsId, handle, LOG_ENTRY_TYPE_FILE, file_name),
                     kvstore(kvstore),
                     tiering_in_agent(tiering_in_agent),
                     meta(initialSize, chunkSize),
                     mode(modeInput),
                     metaDirty(false),
                     data_hold_count(0)
{
}


LogFSFile::~LogFSFile() {}


/*when create file, the caller can set the mode_t parameter*/
void LogFSFile::setTypeFromMode()
{

    if (S_ISREG (mode))
        meta.type = LOGFS_IFREG;
    if (S_ISDIR (mode))
        meta.type = LOGFS_IFDIR;
    if (S_ISLNK (mode))
        meta.type = LOGFS_IFLNK;
    if (S_ISBLK (mode))
        meta.type = LOGFS_IFBLK;
    if (S_ISCHR (mode))
        meta.type = LOGFS_IFCHR;
    if (S_ISFIFO (mode))
        meta.type = LOGFS_IFIFO;
    if (S_ISSOCK (mode))
        meta.type = LOGFS_IFSOCK;

}

void LogFSFile::setProtectionFromMode()
{
    SetProtectionFromMode(mode, &meta.protection);
}

// TODO: belongs in KVFSMetaData.cc, but really this prot stuff should go away
// and just use a mode_t
void SetProtectionFromMode(mode_t mode, meta_prot_t* prot)
{
    //clear to 0 before set
    prot->suid = 0;
    prot->sgid = 0;
    prot->sticky = 0;

    prot->owner.read = 0;
    prot->owner.write = 0;
    prot->owner.exec = 0;

    prot->group.read = 0;
    prot->group.write = 0;
    prot->group.exec = 0;

    prot->other.read = 0;
    prot->other.write = 0;
    prot->other.exec = 0;

    //then update according to mode
    if (mode & S_ISUID)
        prot->suid = 1;
    if (mode & S_ISGID)
        prot->sgid = 1;
    if (mode & S_ISVTX)
        prot->sticky = 1;

    if (mode & S_IRUSR)
        prot->owner.read = 1;
    if (mode & S_IWUSR)
        prot->owner.write = 1;
    if (mode & S_IXUSR)
        prot->owner.exec = 1;

    if (mode & S_IRGRP)
        prot->group.read = 1;
    if (mode & S_IWGRP)
        prot->group.write = 1;
    if (mode & S_IXGRP)
        prot->group.exec = 1;

    if (mode & S_IROTH)
        prot->other.read = 1;
    if (mode & S_IWOTH)
        prot->other.write = 1;
    if (mode & S_IXOTH)
        prot->other.exec = 1;
}

mode_t ModeFromMeta(meta_type_t& type, meta_prot_t& protection) {

    mode_t    st_mode = 0;
    uint32_t  type_bit = 0;
    uint32_t  prot_bit = 0;

    switch (type) {
        case LOGFS_IFREG:
            type_bit = S_IFREG;
            break;
        case LOGFS_IFDIR:
            type_bit = S_IFDIR;
            break;
        case LOGFS_IFLNK:
            type_bit = S_IFLNK;
            break;
        case LOGFS_IFBLK:
            type_bit = S_IFBLK;
            break;
        case LOGFS_IFCHR:
            type_bit = S_IFCHR;
            break;
        case LOGFS_IFIFO:
            type_bit = S_IFIFO;
            break;
        case LOGFS_IFSOCK:
            type_bit = S_IFSOCK;
            break;
        case LOGFS_INVAL:
            break;
    }

    if (protection.suid)
        prot_bit |= S_ISUID;
    if (protection.sgid)
        prot_bit |= S_ISGID;
    if (protection.sticky)
        prot_bit |= S_ISVTX;

    if (protection.owner.read)
        prot_bit |= S_IRUSR;
    if (protection.owner.write)
        prot_bit |= S_IWUSR;
    if (protection.owner.exec)
        prot_bit |= S_IXUSR;

    if (protection.group.read)
        prot_bit |= S_IRGRP;
    if (protection.group.write)
        prot_bit |= S_IWGRP;
    if (protection.group.exec)
        prot_bit |= S_IXGRP;

    if (protection.other.read)
        prot_bit |= S_IROTH;
    if (protection.other.write)
        prot_bit |= S_IWOTH;
    if (protection.other.exec)
        prot_bit |= S_IXOTH;

    st_mode = (type_bit | prot_bit);

    return st_mode;

}

//fill metadata before persistent to log
void LogFSFile::initMeta() {

    meta.metaVersion = 0;   //reserved for upgrade, now = 0;
    meta.checkSum = 0xF5D5;   //reserved, now can be sed to fixed value

    meta.fsId = fsId;
    meta.handle = this->getHandle();

    setTypeFromMode();       //comes from mode_t
    setProtectionFromMode(); //comes from mode_t

    LOG(INFO) << "initMeta: meta.type " << meta.type;
#if 0
    if (meta.type != LOGFS_IFREG)
        meta.type = LOGFS_IFREG;
#endif

    /*need change later*/
    meta.userId = getuid();  //user ID of owner
    meta.groupId = getgid(); //group ID of the owner

    meta.devId = fsId;  //backing device ID
    meta.rDev = 0; //device ID(if special file)

    //fileTotalSize;
    //fileInitialSize;
    //fileChunkSize;

    meta.nLink = 0; // Zero links initially. Increment when links created.

    /* last status change time */
    meta.ctime =  time((time_t*)nullptr);
    meta.ctimeNsec = 0;

    meta.atime = meta.ctime;      /* last access time */
    meta.atimeNsec = 0;
    meta.mtime = meta.ctime;      /* last modification time */
    meta.mtimeNsec = 0;

}

Status LogFSFile::writeToLog()
{
    char key[128];
    snprintf(key, 128, "%lu", handle);
    std::string value;

    value.append(reinterpret_cast<char *>(&(this->meta)),sizeof(LogFSMetaData));

    if(kvstore->put(key, value))
        return Status::STATUS_OK;
    else
        return Status::STATUS_INTERNAL_ERROR;
}


Status
LogFSFile::readFromLog(std::string& readBuffer) {

    //string handleString = format("%lu", handle);
    //Key key(fsId, handleString.c_str(),
    //                                downCast<KeyLength>(handleString.length()));

    Status status = Status::STATUS_OBJECT_DOESNT_EXIST;
    char key[128];
    snprintf(key, 128, "%lu", handle);

    if (kvstore->get(key, readBuffer)) {

        LogFSMetaData* fileMeta = (LogFSMetaData *)readBuffer.c_str();
        if (fileMeta != nullptr) {
            status = Status::STATUS_OK;
        }
        else
            status = Status::STATUS_OBJECT_DOESNT_EXIST;
    }

    return status;
}

Status LogFSFile::create(LogFSFile* parent) {
    Status status = Status::STATUS_OK;
    initMeta();
    //Create file object, and persist to Log.
    VLOG(1) << "LogFSFile::create parent " << parent->handle << " name " <<
        getName() << " ino " << handle;
    int err = parent->insertEntry(handle, getName());
    if (err == 0) {
        NLinkAdjust(1);
        if (meta.type == LOGFS_IFDIR) {
            connectDots(this, parent);  // dot, dotdot
            parent->writeToLog();
        }
        status = writeToLog();
    } else {
        status = Status::STATUS_INTERNAL_ERROR;
    }
    return status;
}

Status LogFSFile::link(LogFSFile* file, const char* name)
{
    Status status = Status::STATUS_OK;
    //Create file object, and persist to Log.
    VLOG(1) << "LogFSFile::link parent " << handle << " file " << file->handle
        << " name " << name;
    int err = insertEntry(file->handle, name);
    if (err == 0) {
        UpdateCTime();
        UpdateMTime();
        file->NLinkAdjust(1); // We just created a link, so increase link count.
        file->UpdateCTime();
        EXPECT(file->meta.type == LOGFS_IFREG);
        status = writeToLog();
        file->writeToLog();
    } else {
        status = Status::STATUS_INTERNAL_ERROR;
    }
    return status;
}

void LogFSFile::NLinkAdjust(int n)
{
    VLOG(1) << "NLinkAdjust i " << handle
        << " " << meta.nLink << " =>  " << meta.nLink + n;
    meta.nLink += n;
}

/*
 * when the file already exists, open the file object to get its meta data and
 * initial data.
 */
bool LogFSFile::open() {

    Status status;
    std::string readBuffer;
    bool ret = false;

    status = readFromLog(readBuffer);
    if(Status::STATUS_OK == status) {
        //if readFromLog return STATUS_OK, then it means the file has been found,
        LogFSMetaData* fileMeta = (LogFSMetaData *)readBuffer.c_str();
        assert(fileMeta->fsId == fsId && fileMeta->handle == handle);
        this->meta = *fileMeta;
        ret = true;
    }
    VLOG(2) << "LogFSFile::open ino " << handle << " ret " << ret;
    return ret;
}


/* Maybe it's better to separte meta and data,
  * or optimize Ramcloud's API to just read part of the value
  * now separate fixed meta and variable meta, so need
  */
Status LogFSFile::stat(struct stat *statBuff)
{

    std::string readBuffer;
    Status status = Status::STATUS_OK;

    status = readFromLog(readBuffer);
    if(Status::STATUS_OK == status)
    {
        LogFSMetaData* fileMeta = (LogFSMetaData *)readBuffer.c_str();

        if(fileMeta->fsId != fsId || fileMeta->handle != handle)
        {
            return Status::STATUS_OBJECT_DOESNT_EXIST;
        }

        this->meta = *fileMeta;

        //fill stat based on metadata
        statBuff->st_dev     = fileMeta->devId;
        statBuff->st_ino     = fileMeta->handle;

        statBuff->st_mode    =
                          ModeFromMeta(fileMeta->type, fileMeta->protection);

        statBuff->st_nlink    = fileMeta->nLink;
        statBuff->st_uid     = fileMeta->userId;
        statBuff->st_gid     = fileMeta->groupId;

        statBuff->st_rdev    = fileMeta->rDev;

        statBuff->st_size    = fileMeta->fileTotalSize;
        statBuff->st_blksize = fileMeta->fileChunkSize;

        // TODO: Holes?
        statBuff->st_blocks  = fileMeta->fileTotalSize/ 512;

        statBuff->st_atime   = fileMeta->atime;
        statBuff->st_mtime   = fileMeta->mtime;
        statBuff->st_ctime   = fileMeta->ctime;

#ifdef HAVE_STRUCT_STAT_ST_ATIM_TV_NSEC
        statBuff->st_atim.tv_nsec = fileMeta->mtimeNsec;
        statBuff->st_mtim.tv_nsec = fileMeta->atimeNsec;
        statBuff->st_ctim.tv_nsec = fileMeta->ctimeNsec;
#endif
    }
    else
        status = Status::STATUS_OBJECT_DOESNT_EXIST;

    return status;
}

// Ugh.
Status LogFSFile::SetAttr(const struct stat* new_attr)
{
    std::string readBuffer;
    Status status = Status::STATUS_OK;

    // Is reading in NECESSARY?? Passed in attributes are complete, but need
    // parent info. Sigh.
    status = readFromLog(readBuffer);
    // TODO: check-valid / Refresh?

    if (Status::STATUS_OK == status) {
        LogFSMetaData* fileMeta = (LogFSMetaData *)readBuffer.c_str();
        if (fileMeta->fsId != fsId || fileMeta->handle != handle
                || new_attr->st_ino != handle) {
            LOG(ERROR) << "LogFSFile::setattr meta mismatch handle " << handle;
            return Status::STATUS_OBJECT_DOESNT_EXIST;
        }
        // What if it's already dirty??
        if (metaDirty) {
            LOG(WARNING) << "Setattr: metaDirty!";
        }
        this->meta = *fileMeta;

        meta.nLink = new_attr->st_nlink;
        meta.userId = new_attr->st_uid;
        meta.groupId = new_attr->st_gid;

        meta.atime = new_attr->st_atime;
        meta.mtime = new_attr->st_mtime;

#ifdef HAVE_STRUCT_STAT_ST_ATIM_TV_NSEC
        meta.mtimeNsec = new_attr->st_atim.tv_nsec;
        meta.atimeNsec = new_attr->st_mtim.tv_nsec;
        meta.ctimeNsec = new_attr->st_ctim.tv_nsec;
#endif
        if (mode != new_attr->st_mode) {
            mode = new_attr->st_mode;
            setProtectionFromMode();
        }
    }
    return status;
}

/*
 * The initial data part of the file locates in the object file,
 * and the remainding data locates in the other file chunk object.
 * the file content must be continous?
 *may need check the return of objectManager->readObject
 */
Status LogFSFile::read(off_t offset, size_t length, char *readBuffer,
        size_t* actualLen, bool needUpdateAtime)
{
    size_t orig_length = length;
    off_t orig_offset = offset;
    Status status = Status::STATUS_OK;
    if (readBuffer == nullptr){
        LOG(WARNING) << "LogFSFile::read null buffer!";
        *actualLen = 0;
        return status;
    }
    VLOG(2) << "LogFSFile::read ino " << handle << " offset " << hex(offset)
        << " length " << hex(length)
        << " fileTotalSize " << hex(meta.fileTotalSize);
    // 
    if ((offset + static_cast<off_t>(length)) >
            static_cast<off_t>(meta.fileTotalSize)) {
        orig_length = meta.fileTotalSize - offset;
        VLOG(2) << "LogFSFile::read limit length from " << length
            << " to " << orig_length;
        length = orig_length;
    }
    HoldData(orig_offset, orig_length);
    if (length > meta.fileInitialSize) {
        off_t first_chunk =
            (offset - meta.fileInitialSize) / meta.fileChunkSize;
        // The chunk containing the last requested byte (hence "- 1").
        off_t last_byte_off =
            offset + static_cast<off_t>(length - 1 - meta.fileInitialSize);
        off_t last_chunk = last_byte_off / meta.fileChunkSize;
        char key[128];
        // NEEDSWORK / TODO: I think the data is being copied at least twice.
        // can we figure out how to have the KVS extract the
        // string straight into the supplied buffer?
        // Or even not copy at all if possible?
        std::string value;
        for (off_t chunk = first_chunk; chunk <= last_chunk; chunk++) {
            off_t chunk_base_off =
                meta.fileInitialSize + chunk * meta.fileChunkSize;
            snprintf(key, 128, "%lu.%lu", handle, chunk);
            if (!kvstore->get(key, value)) {
                // Normal for holes.
                VLOG(2) << "LogFSFile::read - chunk is hole: key " << key;
                value.clear();  // as if the get yielded 0 bytes.
            }
            // value now contains 0 or more bytes of data for file
            // starting at file offset chunk_base_off.
            EXPECT(offset >= chunk_base_off);
            off_t chunk_off = offset - chunk_base_off;
            std::ptrdiff_t avail_bytes = value.size() - chunk_off;
            std::ptrdiff_t copy_bytes =
                std::min(avail_bytes, static_cast<std::ptrdiff_t>(length));
            if (copy_bytes > 0) {
                VLOG(2) << "LogFSFile::read chunk " << chunk
                    << " offset " << hex(offset)
                    << " chunk_off " << hex(chunk_off)
                    << " copy_bytes " << hex(copy_bytes);
                memcpy(readBuffer, value.c_str() + chunk_off,
                        static_cast<size_t>(copy_bytes));
                readBuffer += copy_bytes;
                offset += copy_bytes;
                length -= copy_bytes;
            }
            if (length > 0) {
                // pretend rest of chunk full of zeros.
                off_t next_chunk_base_off = chunk_base_off + meta.fileChunkSize;
                size_t zero_bytes =
                    std::min(length,
                        static_cast<size_t>(next_chunk_base_off - offset));
                VLOG(2) << "LogFSFile::read chunk " << chunk
                    << " zero_bytes " << hex(zero_bytes);
                memset(readBuffer, 0, zero_bytes);
                readBuffer += zero_bytes;
                offset += zero_bytes;
                length -= zero_bytes;
            }
        }
    }
    ReleaseData(orig_offset, orig_length);
    *actualLen = orig_length - length;
    return status;
}

/*
 * when write the data to one file, the file object will be affected:
 * 1/ fileTotalSize will be updated
 * 2/ inital data will be append to the file object
 * 3/ the file content must be continous? double check spare file
 * 4/ !!!note: may need to be optimized to write file object
 *    and file chunk object at the same transaction
 */
Status LogFSFile::write(off_t offset,
                         size_t length,
                         const char *write_buf,
                         size_t* actualLen,
                         bool update_meta)
{
    char key[128];
    const off_t orig_offset(offset);
    Status status = Status::STATUS_OK;
    *actualLen = 0;
    if (write_buf == nullptr) {
        LOG(WARNING) << "LogFSFile::write null buffer!";
        return status;
    }
    off_t first_chunk =
        (offset - meta.fileInitialSize) / meta.fileChunkSize;
    // The chunk containing the last byte to be written (hence "- 1").
    off_t end_off = offset + length;
    off_t last_chunk = (end_off - 1 - meta.fileInitialSize) / meta.fileChunkSize;
    VLOG(2) << "LogFSFile::write off " << hex(offset)
        << " length " << hex(length)
        << " TotalSize " << hex(meta.fileTotalSize)
        << " first chunk " << first_chunk << " last " << last_chunk;
    // NEEDSWORK / TODO: I think the data is being copied at least twice.
    // can we figure out how to have the KVS extract the
    // string straight into the supplied buffer? Or even not copy at all if
    // possible?
    for (off_t chunk = first_chunk; chunk <= last_chunk; chunk++) {
        off_t chunk_base_off =
            meta.fileInitialSize + chunk * meta.fileChunkSize;
        EXPECT(offset >= chunk_base_off);
        // Doing a get of the value might require a disk read (and data copy),
        // so avoid if possible: no need to get the old value if it will be
        // completely overwritten. Note that, since we can't know the size of
        // the chunk without doing the get (sad-face), we can only avoid the get
        // if doing a write that completely covers fileChunkSize.
        bool do_get = true;
        off_t chunk_off = offset - chunk_base_off;
        if (chunk_off == 0) {
            if (length >= meta.fileChunkSize) {
                // writing all of this chunk.
                do_get = false;
            } else if (offset == (off_t)meta.fileTotalSize) {
                // Extending the file. Current value shouldn't exist.
                do_get = false;
            }
        }
        // The key for this chunk in the KVS.
        snprintf(key, 128, "%lu.%lu", handle, chunk);
        std::string value;
        if (do_get) {
            if (!kvstore->get(key, value)) {
                // Normal for holes.
                EXPECT(value.size() == 0);
                value.clear();  // as if the get yielded 0 bytes.
            }
            VLOG(2) << "LogFSFile::write get " << key << " size " <<
                value.size();
        }
        // value now contains 0 or more bytes of data for file
        // starting at file offset chnk_base_off. Write on top of that, then
        // write the new value out.
        // allocate enough bytes in value string.
        off_t chunk_end_off = std::min(offset + (off_t)length,
                chunk_base_off + meta.fileChunkSize);
        std::ptrdiff_t copy_bytes = chunk_end_off - offset;
        VLOG(2) << "LogFSFile::write key " << key << " chunk " << chunk
            << " copy_bytes " << hex(copy_bytes);
        if (copy_bytes > 0) {
            VLOG(2) << " LogFSFile::write replace " << hex(chunk_off)
                << ", " << hex(chunk_end_off - chunk_base_off)
                << ",..., " << hex(copy_bytes);
            // Make sure the value string is long enough for what we're about to
            // write. Insert null bytes for holes.
            // TODO: inefficient because we zero then overwrite the value.
            // Use combo of resize, append, and replace instead.
            if (chunk_end_off - chunk_base_off > (off_t)value.size()) {
                value.resize(chunk_end_off - chunk_base_off);
            }
            value.replace(chunk_off, copy_bytes, write_buf, copy_bytes);
            if (!kvstore->put(key, value)) {
                LOG(ERROR) << "put failed " << key << " size " <<
                    hex(value.size());
                status = Status::STATUS_INTERNAL_ERROR;
                break;
            }
            // TODO: batch this with inode update.
            write_buf += copy_bytes;
            offset += copy_bytes;
            length -= copy_bytes;
        }
    }
    *actualLen = offset - orig_offset;
    if (status == Status::STATUS_OK) {
        if (update_meta) {
            if (offset > (off_t)meta.fileTotalSize) {
                meta.fileTotalSize = offset;
            }

            UpdateMTime();
            UpdateCTime();

            metaDirty = true;  // TODO: This is never actually used.

            status = flush();
            if (status != Status::STATUS_OK) {
                return status;
            }
            metaDirty = false;
        } else if (offset > (off_t)meta.fileTotalSize) {
            // if not update_meta, better not be exceeding file size.
            LOG(DFATAL) << "Write: not extending file size ";
        }
    }
    return status;
}

void LogFSFile::HoldData(off_t offset, size_t size) {
    data_hold_count++;
    VLOG(2) << "HoldData " << handle << " state " << meta.evictionState;
    if (meta.evictionState != ES_RESIDENT) {
        assert(meta.evictionState == ES_DATA_EVICTED);
        UnEvict();
    }
}

// TODO: this only retrieves data. Need recover-meta-data flag.
Status LogFSFile::UnEvict() {
    // request that the file's data be brought back from the cloud.
    DVLOG(2) << "UnEvict: " << handle;
    Status status = Status::STATUS_OK;
    pqfs::TieredInFile* tiered_file = tiering_in_agent->Open(
            meta.handle, pqfs::XID_MAX);
    if (tiered_file == nullptr) {
        LOG(ERROR) << "UnEvict failed " << meta.handle;
        return Status::STATUS_OBJECT_DOESNT_EXIST;
    }
    struct stat stat;
    std::string file_name;
    tiered_file->Stat(&stat, &file_name);
    assert(stat.st_ino == meta.handle);
    assert((off_t)meta.fileTotalSize == stat.st_size);
    /*
    loop over file size,
        Read/map data buffer (chunk) from tiering_file.
        size = tiering_file->ReadData(buf, MIN(sizeof buf, remaining size));
    */
    bool eof = false;
    char buf[8192];
    off_t offset = 0;
    do {
        size_t bytes_read = tiered_file->Read(buf, offset, sizeof(buf));
        if (bytes_read <= 0) {
            eof = true;
            break;
        }
        if (bytes_read > 0) {
            size_t bytes_written = 0;
            Status write_status = this->write(offset, bytes_read, buf,
                &bytes_written, false);  // false = don't update meta data.
            if (write_status != Status::STATUS_OK) {
                LOG(ERROR) << "UnEvict: write error " << write_status;
                status = write_status;
                break;
            }
            assert(bytes_written == bytes_read);
        }
        offset += bytes_read;
    } while (!eof);
    tiered_file->Close();
    delete tiered_file;
    if (status == Status::STATUS_OK) {
        assert(offset == stat.st_size);
        meta.evictionState = ES_RESIDENT;
        flush();
        DVLOG(1) << "UnEvict: restored " << offset << " bytes to i "
                << stat.st_ino << " '"<< file_name << "'";
    } else {
        // TODO: if any data was written, should we delete it?
        LOG(ERROR) << "UnEvict: error status " << status;
    }
    return status;
}

void LogFSFile::ReleaseData(off_t offset, size_t size) {
    data_hold_count--;
}


/*now all write is synchronized, so close can retun directly*/
Status LogFSFile::close() {
    //atime and mtime can be update here
    VLOG(1) << "LogFSFile::close " << handle << " nLink " << meta.nLink;
    if (meta.nLink <= 0) {
        SetSize(0);  // remove any data chunks.
#ifdef KVFS_EXTENDED_ATTRIBUTES
        char fileAttributeKey[128];
        snprintf(fileAttributeKey, 128, "%lu.%s", handle, "attr");
        kvstore->remove(fileAttributeKey);
#endif  // KVFS_EXTENDED_ATTRIBUTES
        RemoveInodeFromKvs();
    }
    return  Status::STATUS_OK;
}

void LogFSFile::RemoveInodeFromKvs() {
    char key[128];
    snprintf(key, 128, "%lu", handle);
    kvstore->remove(key);
}

Status LogFSFile::remove(LogFSFile* parent, const char* name)
{
    uint64_t handle = this->getHandle();

    // delete entry from the parent directory partition
    Status status = Status::STATUS_OK;
    int err = parent->removeEntry(handle, name);
    if (err == 0) {
        NLinkAdjust(-1);
    } else {
        LOG(ERROR) << "LogFSFile::remove: " << parent->handle << " " << name
            << " removeEntry failed.";
        status = Status::STATUS_INTERNAL_ERROR;
    }
    return status;
}

Status LogFSFile::Evict()
{
    struct stat stat;
    Status status = LogFSFile::stat(&stat);
    // TODO(): Inode locking?? What if file is in use?
    // Should this be done via a vnode-op?
    // (This code wouldn't be wrong, but maybe should be called from vnode).
    if (status == Status::STATUS_OK) {
        status = ResizeData(0L, stat.st_size,
                meta.fileChunkSize, meta.fileInitialSize);
        // TODO: For now, we're evicting data, but not the inode itself.
        // RemoveInodeFromKvs();
        metaDirty = true;
        meta.evictionState = ES_DATA_EVICTED;
        flush();
    } else {
        LOG(WARNING) << "Evict: " << handle << " err " << status;
    }
    return status;
}

Status LogFSFile::flush()
{
    char key[128];
    snprintf(key, 128, "%lu", handle);
    std::string value;

    VLOG(2) << "LogFSFile::flush i " << handle << " "  << meta.name
        << " dirty " << metaDirty << " type " << meta.type;
    if (handle != meta.handle) {
        LOG(ERROR) << "LogFSFile::flush handle " << handle
            << " != meta " << meta.handle;
        return Status::STATUS_INTERNAL_ERROR;
    }
    if (!metaDirty) {
        LOG(ERROR) << "TODO: LogFSFile::flush called, metaDirty false";
    }
    value.append(reinterpret_cast<char *>(&(this->meta)),sizeof(LogFSMetaData));
    if (kvstore->put(key, value)) {
        metaDirty = false;
        return Status::STATUS_OK;
    } else {
        return Status::STATUS_INTERNAL_ERROR;
    }
}

/*
     //#define   F_OK        0   // test for existence of file
    //#define   X_OK        0x01    //test for execute or search permission
    //#define   W_OK        0x02    // test for write permission
    //#define   R_OK        0x04    //test for read permission

     On success (all requested permissions granted, or mode is F_OK and
     the file exists), zero is returned.  On error (at least one bit in
     mode asked for a permission that is denied, or mode is F_OK and the
     file does not exist, or some other error occurred), -1 is returned,
     and errno is set appropriately.
 */
Status LogFSFile::access(int mode,
                           const uint32_t userId,
                           const uint32_t groupId,
                           int *pEerror) {

    Status status = Status::STATUS_OK;

    bool ok = true;

    //check the permission: owner, group, others
    if(userId == meta.userId)
    {
        if(mode & X_OK)
        {
            if(meta.protection.owner.exec != 1)
                ok = false;
        }

        if(mode & W_OK)
        {
            if(meta.protection.owner.write != 1)
                ok = false;
        }

        if(mode & R_OK)
        {
            if(meta.protection.owner.read != 1)
                ok = false;
        }
    }
    else if(groupId == meta.groupId)
    {
        if(mode & X_OK)
        {
            if(meta.protection.group.exec != 1)
                ok = false;
        }

        if(mode & W_OK)
        {
            if(meta.protection.group.write != 1)
                ok = false;
        }

        if(mode & R_OK)
        {
            if(meta.protection.group.read != 1)
                ok = false;
        }
    }
    else
    {
        if(mode & X_OK)
        {
            if(meta.protection.other.exec != 1)
                ok = false;
        }

        if(mode & W_OK)
        {
            if(meta.protection.other.write != 1)
                ok = false;
        }

        if(mode & R_OK)
        {
            if(meta.protection.other.read != 1)
                ok = false;
        }

    }

    if(!ok) {
        *pEerror = EACCES;
        status = Status::STATUS_INVALID_PARAMETER;
    }

    return status;
}

Status LogFSFile::chmod(mode_t mode)
{
    //change the permissions of the file according to the input mode
    this->mode = mode;
    setProtectionFromMode();
    //persist the changed permission to the basic file obejct
    return flush();
}

Status LogFSFile::chown(const uint32_t userId, const uint32_t groupId)
{
    Status status = Status::STATUS_OK;
    //change userId and groupId  of the file according to the input
    meta.userId = userId;
    meta.groupId = groupId;
    //persist the changed permission to the basic file obejct
    status = flush();
    return status;
}

Status LogFSFile::SetSize(off_t new_size)
{
    Status status = Status::STATUS_OK;
    if (!open()) {  // TODO: necessary? If so, why not necessary for other ops?
        return Status::STATUS_INTERNAL_ERROR;
    }
    const off_t& old_size = meta.fileTotalSize;
    status = ResizeData(new_size, old_size, meta.fileChunkSize, meta.fileInitialSize);

    if (status == Status::STATUS_OK) {
        meta.fileTotalSize = new_size;
        meta.mtime = meta.ctime = time((time_t*)nullptr);
        meta.mtimeNsec = meta.ctimeNsec = 0;
        metaDirty = true;  // TODO: This is never actually used.
        status = flush();
        if(status == Status::STATUS_OK) {
            metaDirty = false;
        }
    }
    return status;
}

/*
 * Removes data chunks, handles partial chunks.
 * Used for truncation, deletion, and eviction.
 */
Status LogFSFile::ResizeData(
        off_t new_size,
        off_t old_size,
        off_t file_chunk_size,
        off_t file_initial_size)
{
    LOG(INFO) << "ResizeData: new_size " << new_size << " old_size " << old_size;
    Status status = Status::STATUS_OK;
    if (new_size < old_size) {
        char chunk_key[128];
        // first chunk that may be affected.
        off_t first_chunk = (new_size - file_initial_size) / file_chunk_size;
        // The chunk containing the last affected byte (hence "- 1").
        off_t last_byte = old_size - 1;
        assert(last_byte >= 0);
        off_t last_chunk = (off_t)(last_byte - file_initial_size)
            / file_chunk_size;
        VLOG(2) << "ResizeData new " << hex(new_size) << " old " << hex(old_size)
            << " chunks " << first_chunk << ".." << last_chunk;
        // Note: last_chunk may be -1 here if file already zero sized
        // (i'm depending on fact that off_t is signed).
        for (off_t chunk = first_chunk; chunk <= last_chunk; chunk++) {
            // Either delete or shorten affected chunks.
            off_t chunk_base_off =
                file_initial_size + chunk * file_chunk_size;
            off_t chunk_off = new_size - chunk_base_off;
            snprintf(chunk_key, 128, "%lu.%lu", handle, chunk);
            if (chunk == first_chunk && chunk_off > 0
                    && chunk_off < file_chunk_size) {
                std::string value;
                VLOG(2) << "ResizeData " << chunk_key << " resize to " <<
                    hex(chunk_off);
                if (kvstore->get(chunk_key, value)
                        && (value.size() > (size_t)chunk_off)) {
                    value.resize(chunk_off);
                    if (!kvstore->put(chunk_key, value)) {
                        LOG(ERROR) << "LogFSFile::ResizeData: put "
                            << chunk_key << " size "
                            << value.size() << " failed.";
                        status = Status::STATUS_INTERNAL_ERROR;
                    }
                }
            } else {
                VLOG(2) << "ResizeData chunk remove " << chunk_key;
                kvstore->remove(chunk_key);
            }
        }
    }
    return status;
}

bool
LogFSFile::exists() {
    std::string outBuffer;
    Status status = readFromLog(outBuffer);
    bool ret = false;
    if (status == Status::STATUS_OK) {
        LogFSMetaData* fileMeta = (LogFSMetaData *)outBuffer.c_str();
        if (fileMeta != nullptr) {
            if(fileMeta->fsId == fsId && fileMeta->handle == handle)
            {
                return true;
            }
        }
    }
    return ret;
}

const int DEFAULT_PARTITIONS_PER_DIRECTORY = 2;

size_t LogFSFile::get_num_partitions() {
    if (_partitions == nullptr) 
        return DEFAULT_PARTITIONS_PER_DIRECTORY;
    return _partitions->size();
}

// get or allocate the specified partition.
LogFSDirectoryPartition* LogFSFile::GetPartition(int p)
{
    LogFSDirectoryPartition* part;
    if (_partitions == nullptr) {
        _partitions.reset(
                new std::vector<std::unique_ptr<LogFSDirectoryPartition>>(
                    DEFAULT_PARTITIONS_PER_DIRECTORY));
    }
    if ((part = _partitions->at(p).get()) == nullptr) {
        part = new LogFSDirectoryPartition(p, fsId, handle, kvstore);
        _partitions->at(p).reset(part);
    }
    return part;
}

LogFSDirectoryPartition* LogFSFile::GetPartition(const std::string& name) {
    size_t p = CityHash64(name.c_str(), name.length()) % get_num_partitions();
    return GetPartition(p);
}

void LogFSFile::ReleasePartition(LogFSDirectoryPartition* partition)
{
    // TODO:
    VLOG(3) << "ReleasePartition NYI " << partition->getPartitionNumber();
}

uint64_t
LogFSFile::findHandle(const std::string& name) {
    auto* partition = GetPartition(name);
    return partition->findEntry(name);
}

// So readdir "offsets" doesn't have to rescan. high-order part of offset is the
// partition number.
static const off_t MAX_ENTRIES_PER_PARTITION = 1e6;

Status
LogFSFile::listDirectory(
        DirEntryAdder* dir_entry_adder,
         off_t cur_offset)
{
    if (!exists()) {
        LOG(WARNING) << "listDirectory: ! exists handle " << handle;
        return Status::STATUS_OBJECT_DOESNT_EXIST;
    }
    // Start with partition indicated by offset.
    for (uint16_t i = cur_offset / MAX_ENTRIES_PER_PARTITION;
            i < get_num_partitions()
            && !dir_entry_adder->get_out_of_space();
            i++)
    {
        // Can this partition contain entries with useful offsets
        off_t part_base_offset = i * MAX_ENTRIES_PER_PARTITION;
        if (cur_offset < part_base_offset) {
            cur_offset = part_base_offset;
        }
        auto* partition = GetPartition(i);
        int err = partition->listEntries(
                dir_entry_adder, cur_offset, part_base_offset);
        EXPECT(err == 0);
        ReleasePartition(partition);
    }
    return Status::STATUS_OK;
}

bool
LogFSFile::isDirEmpty() {
    bool isEmpty = true;
    for (size_t i = 0; i < get_num_partitions() && isEmpty; i++) {
        auto* partition = GetPartition(i);
        isEmpty = partition->isPartitionEmpty();
        ReleasePartition(partition);
    }
    return isEmpty;
}

Status
LogFSFile::insertEntry(const uint64_t handle, const std::string& name)
{
    Status status = Status::STATUS_OK;
    auto* partition = GetPartition(name);
    int err = partition->insertEntry(handle, name);
    if (err != 0)
        status = Status::STATUS_INTERNAL_ERROR;
    ReleasePartition(partition);
    return status;
}

Status
LogFSFile::removeEntry(const uint64_t handle, const std::string& name)
{
    Status status = Status::STATUS_OK;
    auto* partition = GetPartition(name);
    int err = partition->removeEntry(handle, name);
    if (err != 0)
        return Status::STATUS_INTERNAL_ERROR;
    ReleasePartition(partition);
    return status;
}

// Create the "." and ".." entries in this directory.
Status
LogFSFile::connectDots(LogFSFile* dot, LogFSFile* dotdot)
{
    assert(dot == this);
    VLOG(1) << "connectDots dot " << dot->handle
        << " dotdot " << dotdot->handle;
    Status status = insertEntry(dot->handle, ".");
    if (status == Status::STATUS_OK) {
        dot->NLinkAdjust(1);
        metaDirty = true;
        status = insertEntry(dotdot->handle, "..");
        if (status == Status::STATUS_OK) {
            dotdot->NLinkAdjust(1);
            dotdot->metaDirty = true;
        }
    }
    return status;
}

Status
LogFSFile::createRoot() {
    initMeta();
    connectDots(this, this);
    Status status = writeToLog();
    VLOG(2) << "LogFSFile::createroot status " << status;
    return status;
}

// TODO: combine thes e and add flag bits to specify which time(s) to update.
// (see also FUSE_SETATTR_MTIME, etc).
time_t LogFSFile::UpdateMTime(time_t new_time)
{
    meta.mtime = new_time;
    meta.mtimeNsec = 0;
    return new_time;
}

time_t LogFSFile::UpdateCTime(time_t new_time)
{
    meta.ctime = new_time;
    meta.ctimeNsec = 0;
    return new_time;
}

time_t LogFSFile::UpdateATime(time_t new_time)
{
    meta.atime = new_time;
    meta.atimeNsec = 0;
    return new_time;
}
