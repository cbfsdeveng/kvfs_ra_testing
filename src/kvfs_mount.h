#include "cloud_proxy.h"
#include <memory>
#include "expect.h"
#include "FileSystemManager.h"
#include "file_test_util.h"
#include "fs_change_log.h"
#include <glog/logging.h>
#ifdef PROTOBUF_V3              // nice to have, hassle to switch?
#include <google/protobuf/util/message_differencer.h>
#endif
#include "hexdump.h"
#include "index_manager.h"
#include "index.pb.h"
#include "KVFSFile.h"
#include "KVStore.h"
#include "super.pb.h"
// NEEDSWORK: IWYU

#ifndef KVFS_MOUNT_
#define KVFS_MOUNT_
const uint64_t TEST_FSID = 0;

class KvfsMount;

// NEEDSWORK! FuseKvfsInode
class FuseKvfsInode {
public:
    // state of our struct stat (attr) relative to the underlying file.
    enum State {
        INVALID,  // our attr is invalid, must stat underlying file.
        CLEAN,    // our attr is valid, no need to stat underlying file.
        DIRTY     // our attr is dirty - need flush to underlying file
    };

    FuseKvfsInode(KvfsMount* mount, ino_t ino);

    ~FuseKvfsInode() {
    }

    // Hack.
    int Getattr(struct stat* attr);
    int Evict();

    // TODO: these should take pointer to req instead of ref, since not const.
    void Getattr(fuse_req_t& req, fuse_file_info* fi);
    void Setattr(fuse_req_t& req, const struct stat& attr,
                 int to_set, struct fuse_file_info *fi);
    void Read(fuse_req_t& req, size_t size, off_t off,
            struct fuse_file_info *fi);
    void Readdir(fuse_req_t& req, size_t size, off_t offset,
            struct fuse_file_info *fi);
    void Write(fuse_req_t& req, const char *buf, size_t size, off_t off,
            struct fuse_file_info *fi);
    void Flush(fuse_req_t& req, struct fuse_file_info *fi);
    void Lookup(fuse_req_t& req, const char *name);
    void Create(fuse_req_t& req, const char *name, mode_t mode,
            struct fuse_file_info *fi);
    void Mkdir(fuse_req_t& req, const char *name, mode_t mode);
    void Rmdir(fuse_req_t& req, const char *name);
    void Link(fuse_req_t& req, ino_t ino, const char* name);

    void Unlink(fuse_req_t& req, const char *name);

    int64_t Hold(int n, const char* whence);
    int64_t Release(int n, const char* whence);
    const uint64_t get_fs_id();
    const State get_state() { return state; }
    std::string get_name() { return file.getName(); }
    pqfs::xid_t get_xid() { return file.get_xid(); }

// private:  -- should be private.
    struct stat attr;  // Called attr to be consistent with fuse api.
private:
    // prevent compiler from generating.
    FuseKvfsInode();  // suppress default constructor
    FuseKvfsInode(const FuseKvfsInode&);  // and copy constructor
    FuseKvfsInode& operator=(const FuseKvfsInode&);  // and assignment

    void set_state(State new_state, const char* whence);
    int Refresh(const char* whence = "");

    void set_name(std::string name) { file.setName(name); }
    void set_mode(mode_t mode) { file.setMode(mode); attr.st_mode = mode; }
    mode_t get_mode() { return attr.st_mode; }

    KvfsMount* kvfs_mount;  // Not owned.
    LogFSFile file;
    int64_t nlookup;
    State state;
};


#include <fuse_lowlevel.h>
#include <dirent.h>
#include <sys/stat.h>

namespace pqfs {
class TieringInAgent;
}

// The state of a mount of a KVFS filesystem.
// (NEEDSWORK: due to fsid, possibly several, but there should probably just be
// one KvfsMount per fsid).
class KvfsMount {
public:
    // key-value store.
    // store name
    // proxy info
    KvfsMount(KVStore* store,  // not owned.
            const std::string& store_name,
            uint64_t fs_id,
            pqfs::CloudProxy* cloud_proxy);

    void ArgParse();          // ?

    int Init(FileSystemManager* filesystem_manager);
    void set_tiering_in_agent(pqfs::TieringInAgent* tiering_in_agent);

    void Shutdown();

    void Start();

    void Stop();

    int Wait();

    int ReadSuperblock();
    int WriteSuperblock();

    void FlushLog();

    void Join();

    void FreeInode(FuseKvfsInode *inode);

    // get (or optionally create) the specified inode from our inode map.
    // TODO: locking + return inode held (caller to release).
    FuseKvfsInode* GetInode(ino_t ino, bool create, const char* whence);

    // Allocate the next inode number.
    fuse_ino_t GetNextIno();

    void DumpInodeMap();

    FileSystemManager* get_fs_manager() { return filesystem_manager; } 
    KVStore* get_store() { return store; }
    pqfs::TieringInAgent* get_tiering_in_agent() { return tiering_in_agent; }
    pqfs::FsChangeLog* get_change_log() { return change_log.get(); }
    void set_fuse_attr_timeout(bool attr_timeout) { fuse_attr_timeout = attr_timeout; }
    void set_fuse_direct_io(bool direct_io) { fuse_direct_io = direct_io; }
    void set_fuse_entry_timeout(bool entry_timeout) { fuse_entry_timeout = entry_timeout; }
    void set_fuse_keep_cache(bool keep_cache) { fuse_keep_cache = keep_cache; }
    const bool get_fuse_direct_io() { return fuse_direct_io; }
    const bool get_fuse_keep_cache() { return fuse_keep_cache; }
    const double get_fuse_attr_timeout() { return fuse_attr_timeout; }
    const double get_fuse_entry_timeout() { return fuse_entry_timeout; }
    const size_t get_chunk_size() { return chunk_size; }
    const uint64_t get_fs_id() { return fs_id; }

private:
    FileSystemManager* filesystem_manager;  // Not owned.
    KVStore* store;  // Not owned.
    std::string store_name;
    pqfs::TieringInAgent* tiering_in_agent;  // Not owned.
    pqfs::CloudProxy* cloud_proxy;  // Not owned.
    std::unique_ptr<pqfs::FsChangeLog> change_log;  // Owned
    // std::unique_ptr<pqfs::WebController> web_controller;  // Owned.
    pqfs::proto::Superblock super;
    uint64_t fs_id;
    // Table of active inodes.
    std::unordered_map<ino_t, FuseKvfsInode> inode_map;
    // Control fuses caching.
    bool fuse_keep_cache = true;
    bool fuse_direct_io = false;
    // If true, retrieve index and tarballs from the cloud on startup.
    bool sync_from_cloud = false;
    size_t chunk_size;
    double fuse_attr_timeout;
    double fuse_entry_timeout;
};


#endif  // KVFS_MOUNT_
