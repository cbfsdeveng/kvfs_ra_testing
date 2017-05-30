/*
 * KVFSMetaData.h
 *
 *  Created on: Oct 15, 2015
 *      Author: Prasanna Ponnada
 */

/*
#include <stdint.h>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
*/


#ifndef KVFS_METADATA
#define KVFS_METADATA

#include <glog/logging.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>  
#include <limits.h>    // NAME_MAX
#include <sys/xattr.h>
#include <unistd.h>
#include "fs_change_log.h"  // IWYU: For xid_t

#define DEFAULT_FILE_INITIAL_SIZE  0 //file object
//#define DEFAULT_FILE_CHUNK_SIZE  (64*1024)   //file chunk object
#define DEFAULT_FILE_CHUNK_SIZE  (8*1024)   //file chunk object

#if (DEFAULT_FILE_INITIAL_SIZE > DEFAULT_FILE_CHUNK_SIZE)
#define DEFAULT_STUFF_DATA_SIZE DEFAULT_FILE_INITIAL_SIZE
#else
#define DEFAULT_STUFF_DATA_SIZE DEFAULT_FILE_CHUNK_SIZE
#endif

enum meta_type_t {
        LOGFS_INVAL = 0,
        LOGFS_IFREG,
        LOGFS_IFDIR,
        LOGFS_IFLNK,
        LOGFS_IFBLK,
        LOGFS_IFCHR,
        LOGFS_IFIFO,
        LOGFS_IFSOCK
};

enum EvictionState {
    ES_RESIDENT = 0,
    ES_DATA_EVICTED = 1,
    ES_EVICTED = 2
};

struct meta_prot_t {
        uint8_t    suid:1;
        uint8_t    sgid:1;
        uint8_t    sticky:1;
        struct {
                uint8_t    read:1;
                uint8_t    write:1;
                uint8_t    exec:1;
        } owner, group, other;
};

void SetProtectionFromMode(mode_t mode, meta_prot_t* prot);

mode_t ModeFromMeta(meta_type_t& type, meta_prot_t& protection);

struct LogFSMetaData{

        uint32_t       metaVersion;   //reserved for upgrade, now = 0;
        uint32_t       checkSum;   //reserved, now can be sed to fixed value

        uint64_t       fsId;
        uint64_t       handle; //handle is the unique inode number 
        pqfs::xid_t          xid;

        /*or just keep mode_t directly?*/
        meta_type_t    type;       //comes from mode_t 
        meta_prot_t    protection; //comes from mode_t 

        uint32_t       userId;  //user ID of owner
        uint32_t       groupId; //group ID of the owner

        uint64_t       devId;  //backing device ID
        uint64_t       rDev; //device ID(if special file)

        uint64_t       fileTotalSize;
        enum EvictionState evictionState;
        
        // TODO: remove intial-size stuff for now. It's for allowing small files
        // to live in the inode, which we're not doing for now.
        uint32_t       fileInitialSize; 
        uint32_t       fileChunkSize;

        uint32_t       nLink; //link count

        uint32_t       atime;      /* last access time */
        uint32_t       atimeNsec;
        uint32_t       mtime;      /* last modification time */
        uint32_t       mtimeNsec;
        uint32_t       ctime;      /* last status change time */
        uint32_t       ctimeNsec;   

#define PATH_INFO_IN_META
#ifdef PATH_INFO_IN_META
        // for inode-to-pathname mapping (simplistic single-name version):
        // (This should actually be 1 or more records, and the name should be
        // variable length - need protobufs for that).
        uint64_t parentHandle;
        // TODO: wastes space (though, compressed by leveldb).
        char name[NAME_MAX];

        int addPathInfo(uint64_t parent_handle, const std::string& aname) {
            if (aname.size() >= sizeof(name)) {
                LOG(FATAL) << "addPathInfo: name '" << aname << "' too long";
            } 
            parentHandle = parent_handle;
            strncpy(name, aname.c_str(), sizeof(name) - 1);
            return 0;
        }
        int getPathInfo(uint64_t* parent_handle, std::string* aname) const {
            *parent_handle = parentHandle;
            aname->assign(name);
            return 0;
        }
#endif
        void set_xid(pqfs::xid_t new_xid) {
            xid = new_xid;
        }
        const pqfs::xid_t get_xid() {
            return xid;
        }

        LogFSMetaData(
                uint32_t initialSize = DEFAULT_FILE_INITIAL_SIZE,
                uint32_t chunkSize = DEFAULT_FILE_CHUNK_SIZE):
         metaVersion(0),
         checkSum(0),
         fsId(0),
         handle(0),
         xid(0),
         type(LOGFS_INVAL),
         protection(),
         userId(0),
         groupId(0),
         devId(0),
         rDev(0),
         fileTotalSize(0),
         evictionState(ES_RESIDENT),
         fileInitialSize(initialSize),
         fileChunkSize(chunkSize),
         nLink(0),
         atime(0),      
         atimeNsec(0),
         mtime(0),
         mtimeNsec(0),
         ctime(0),      
         ctimeNsec(0)
#ifdef PATH_INFO_IN_META
    ,
    parentHandle(0),
    name{0,}
#endif
    { }
} ;

#ifdef NOT_USED_WTF
struct LogFSAttribute {
        uint64_t       fileTotalSize;   
        
        uint32_t       atime;      /* last access time */
        uint32_t       atimeNsec;
        
        uint32_t       mtime;      /* last modification time */
        uint32_t       mtimeNsec;  

        uint64_t       attrTotalSize;

        LogFSAttribute():
        fileTotalSize(0), 
        atime(0),
        atimeNsec(0),
        mtime(0),
        mtimeNsec(0),
        attrTotalSize(0) {} 
};
#endif

enum LogEntryType {
    /// Invalid log entry. This type should never be used.
    LOG_ENTRY_TYPE_INVALID = 0,

    /// See LogMetadata.h::SegmentHeader
    LOG_ENTRY_TYPE_SEGHEADER,

    /// See Object.h::Object
    LOG_ENTRY_TYPE_OBJ,

    /// See Object.h::ObjectTombstone
    LOG_ENTRY_TYPE_OBJTOMB,

    /// See LogDigest
    LOG_ENTRY_TYPE_LOGDIGEST,

    /// See Object.h::ObjectSafeVersion
    LOG_ENTRY_TYPE_SAFEVERSION,

    /// See TableStats.h::Digest
    LOG_ENTRY_TYPE_TABLESTATS,

    LOG_ENTRY_TYPE_DIR,

    LOG_ENTRY_TYPE_DIR_METADATA,

    LOG_ENTRY_TYPE_DIR_DIGEST,

    LOG_ENTRY_TYPE_FILE,

    LOG_ENTRY_TYPE_FILE_CHUNK,

    LOG_ENTRY_TYPE_FILE_ATTR,

    LOG_ENTRY_TYPE_FILE_DIGEST,

    LOG_ENTRY_TYPE_FILE_SNAPSHOT,

    LOG_ENTRY_TYPE_DIR_SNAPSHOT,

    LOG_ENTRY_TYPE_FILE_CLONE,

    LOG_ENTRY_TYPE_DIR_CLONE,

    LOG_ENTRY_TYPE_OBJ_HANDLE,

    LOG_ENTRY_TYPE_DIR_PARTITION,

    /// Not a type, but rather the total number of types we have defined.
    /// This is currently restricted by the lower 6 bits in a uint8_t field
    /// in Segment.h's Segment::EntryHeader. RAMCloud will probably collapse
    /// under it's own complexity before we exceed 64 types.
    TOTAL_LOG_ENTRY_TYPES
};


#endif /* KVFS_METADATA */
