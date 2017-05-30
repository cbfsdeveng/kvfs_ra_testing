#include "cloud_proxy.h"
#include "expect.h"
#include "FileSystemManager.h"
#include "file_test_util.h"
#include "fs_change_log.h"
#include <glog/logging.h>
#include "index_manager.h"
#include "index.pb.h"
#include "KVStore.h"
#include "super.pb.h"
//#include "tiering_out_agent.h"
//#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>


#ifdef PROTOBUF_V3              // nice to have, hassle to switch?
#include <google/protobuf/util/message_differencer.h>
#endif

#include "kvfs_mount.h"

// Short for debugging.
const double  DEFAULT_FUSE_TIMEOUT = 20.;

// The state of a mount of a KVFS filesystem.
// one KvfsMount per fsid.
// leveldb name
// proxy info
KvfsMount::KvfsMount(KVStore* store,  // not owned.
        const std::string& store_name,
        uint64_t fs_id,
        pqfs::CloudProxy* cloud_proxy
        ) :
    filesystem_manager(nullptr),
    store(store),
    cloud_proxy(cloud_proxy),
    change_log(nullptr),
    fs_id(fs_id),
    fuse_keep_cache(true),
    chunk_size(4096),   // TODO
    fuse_attr_timeout(DEFAULT_FUSE_TIMEOUT),
    fuse_entry_timeout(DEFAULT_FUSE_TIMEOUT)
{
    this->store_name = store_name;
}
    
void KvfsMount::ArgParse() {
    // log ...
}

int KvfsMount::Init(FileSystemManager* filesystem_manager) {
    int err = 0;
    this->filesystem_manager = filesystem_manager;
    if (!store->open(store_name)) {
        LOG(FATAL) << "KvfsMount::Init couldn't open key value store '"
            << store_name << "'";
    }
    // TODO: move to top level (main / mount factory). Used by tiering, not fs.
    cloud_proxy->Init();

    if ((err = ReadSuperblock()) != 0) {
        LOG(FATAL) << "KvfsMount::Init failed to read superblock, err " << err;
    }

    if (sync_from_cloud) {
	// Sync from the cloud back to our proxy dir.
	cloud_proxy->Sync(pqfs::CloudProxy::SYNC_IN);
    }
    // Single change log for now. TODO: allocate one per filesystem.
    change_log.reset(new pqfs::FsChangeLog(cloud_proxy->get_local_dir()));
    change_log->set_next_xid(super.next_xid());
    change_log->Init();
    EXPECT(filesystem_manager->mountFileSystem(store_name, change_log.get()));
    // Create restful web-controller 
    // TODO(joe): local config file
    const char *port = getenv("WEB_SERVER_PORT");
    if (port == nullptr) {
	LOG(FATAL) << "WEB_SERVER_PORT not specified.";
    }
    int port_num = atoi(port);
    if (port_num <= 1024) {
        LOG(FATAL) << "Invalid WEB_SERVER_PORT "
		<< port << "(" << port_num << ")";
    }
    return err;
}

void KvfsMount::set_tiering_in_agent(pqfs::TieringInAgent* new_tiering_in_agent)
{
    tiering_in_agent = new_tiering_in_agent;
    filesystem_manager->setTieringInAgent(tiering_in_agent);
}


using namespace google::protobuf;
using namespace google::protobuf::io;

// TODO: These superblock operations could be moved to a manager class..
int KvfsMount::ReadSuperblock()
{
    int err = 0;
    std::string super_key("SUPER." + std::to_string(fs_id));
    std::string super_value;
    if (!store->get(super_key, super_value)) {
        LOG(INFO) << "ReadSuperblock: none present, initializing fsid " << fs_id;
        // Since we don't yet have an explicit, separate mkfs, not finding our
        // superblock meands we should create one.
        // For now, just initialize fields here.
        super.set_magic(17);
        super.set_filesystem_name("PQFS_test");
        super.set_cloud_uri(cloud_proxy->CloudUri(""));
        super.set_next_inode(FUSE_ROOT_ID + 1);  // ?
        super.set_next_xid(1);
        super.set_fs_id(fs_id);
        super.set_dev(0);  // TODO: FUSE provides the pseudo-dev for mounts.
        super.set_chunk_size(4096);
        // Hack:
        WriteSuperblock();
    } else {
        LOG(INFO) << "ReadSuperblock: got " << super_value;
        // std::istringstream(super_value);
        if (!TextFormat::MergeFromString(super_value, &super)) {
            err = errno ? errno : EIO;
        }
        filesystem_manager->SetNextInode(super.next_inode());
    }
    LOG(INFO) << "ReadSuperblock: " << super.DebugString();
    return err;
}

// TODO: These superblock operations could be moved to a manager class..
int KvfsMount::WriteSuperblock()
{
    int err = 0;
    super.set_next_inode(filesystem_manager->GetCurrentInode() + 1000);
    // TODO: rewrite when we've used up the 100 (we write on clean exit).
    std::string super_key("SUPER." + std::to_string(fs_id));
    std::string super_value;
    if (!TextFormat::PrintToString(super, &super_value)) {
            err = errno ? errno : EIO;
    }
    if (!err) {
        if (!store->put(super_key, super_value)) {
            LOG(WARNING) << "WriteSuperblock: put failed! fsid " << fs_id;
            err = EIO;
        }
    }
    return err;
}

void KvfsMount::Shutdown() {
    WriteSuperblock();
    // NEEDSWORK: necessary? Useful?
    cloud_proxy->Sync(pqfs::CloudProxy::SYNC_OUT);

    if (VLOG_IS_ON(1)) {
        DumpInodeMap();
        store->dump(LOG(INFO));
    }
    Stop();
    FlushLog();
    Join();
    filesystem_manager->umountFileSystem();
}

void KvfsMount::Start() {
    // tiering_out_agent->Start();
}

void KvfsMount::Stop() {
    // tiering_out_agent->RequestStop();
    // web_controller->RequestStop();
}

int KvfsMount::Wait() {
    // tiering_out_agent->Wait(0, 1);  // Wait up to 1 sec to get to xid 100.
    return 0;
}

void KvfsMount::FlushLog() {
    change_log->Flush();
}

void KvfsMount::Join() {
    // web_controller->Join();
    // Wait for the agent to terminate.
    // tiering_out_agent->Join();
}

FuseKvfsInode*
KvfsMount::GetInode(ino_t ino, bool create, const char* whence)
{
    FuseKvfsInode* inode = nullptr;
    bool created = false;
    //std::unordered_map<ino_t, FuseKvfsInode>::const_iterator ino_iter
    // lock inode_map
    if (create) {
        // Construct the entry in-place in the map.
        // See http://en.cppreference.com/w/cpp/utility/tuple/forward_as_tuple
        // for explanation of this emplace syntax.
        auto iter_bool = inode_map.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(ino),
                std::forward_as_tuple(this, ino));
        inode = &(iter_bool.first->second);
        created = iter_bool.second;
    } else {
        auto ino_iter = inode_map.find(ino);
        if (ino_iter != inode_map.end()) {
            inode = &ino_iter->second;
        }
    }
    if (inode != nullptr) {
        inode->Hold(1, "GetInode");
    }
    // TODO: unlock inode_map.
    VLOG(1) << "GetInode from " << whence << " ino " << ino
        << " created=" << created << " inodep " << inode
        << " map size " << inode_map.size();
    if (VLOG_IS_ON(2) && created) {
        DumpInodeMap();
    }
    return inode;
}

// TODO: When called?
void KvfsMount::FreeInode(FuseKvfsInode* inode) {
    assert(inode != nullptr);
    int erase_count = inode_map.erase(inode->attr.st_ino);
    VLOG(2) << "FreeInode: inode_map size " << inode_map.size()
        << " ino " << inode->attr.st_ino << " erased " << erase_count;
}

// Allocate the next inode number.
fuse_ino_t KvfsMount::GetNextIno()
{
    // TODO: move getNextHandleId code and state to here.
    return filesystem_manager->getNextHandleId();
}

void KvfsMount::DumpInodeMap()
{
    FuseKvfsInode* inodep;
    LOG(INFO) << "DumpInodeMap: size " << inode_map.size();
    for (auto ino_iter = inode_map.begin();
            ino_iter != inode_map.end();
            ino_iter++)
    {
        inodep = &ino_iter->second;
        LOG(INFO) << "ino " << inodep->attr.st_ino
            << " name " << inodep->get_name()
            << " xid " << inodep->get_xid()
            << " nlookup " << inodep->Hold(0, "");
    }
}

// ============================================================================

FuseKvfsInode::FuseKvfsInode(
        KvfsMount* mount, ino_t ino) :
    kvfs_mount(mount),
    file(mount->get_fs_id(),
            nullptr,  // name
            ino,
            mount->get_store(),
            mount->get_tiering_in_agent()), 
    nlookup(0),  // Starts out held. Release after done.
    state(INVALID)
{
    // uint32_t initialSize = DEFAULT_FILE_INITIAL_SIZE,
    // uint32_t chunkSize = DEFAULT_FILE_CHUNK_SIZE;
    attr.st_ino = ino;
    attr.st_dev = 0;  // Not used: FUSE fakes a dev for us.
}

int64_t FuseKvfsInode::Hold(int n, const char* whence)
{
    if (n != 0) {
        nlookup += n;
        // n == 0 is just a get of nlookup.
        VLOG(2) << "FuseKvfsInode::Hold from " << whence
            << " ino " << attr.st_ino << " n " << n << " nlookup now " << nlookup;
    }
    return nlookup;
}

int64_t FuseKvfsInode::Release(int n, const char* whence)
{
    VLOG(3) << "FuseKvfsInode::Release from " << whence
        << " ino " << attr.st_ino << " nlookup " << nlookup << " n " << n;
    if (n > nlookup) {
        LOG(ERROR) << "FuseKvfsInode::Release n " << n
            << " nlookup " << nlookup;
        nlookup = 0;
    } else {
        nlookup -= n;
    }
    if (nlookup == 0) {
        VLOG(2) << "FuseKvfsInode::Release from " << whence
            << " close " << attr.st_ino;
        file.close();
    }
    return nlookup;
}

// TODO: move to kvfs_inode.cc
void FuseKvfsInode::Getattr(fuse_req_t& req, fuse_file_info* fi)
{
    int err = Refresh("Getattr");
    if (err != 0) {
        VLOG(2) << "FuseKvfsInode::Getattr err " << err;
        fuse_reply_err(req, err);
    } else {
        VLOG(2) << "FuseKvfsInode::Getattr ino " << attr.st_ino
                << " mode " << std::oct << attr.st_mode;
        fuse_reply_attr(req, &attr, kvfs_mount->get_fuse_attr_timeout());
    }
}

// TODO: move to kvfs_inode.cc
// TODO: temporary hack overload!
int FuseKvfsInode::Getattr(struct stat* attrp)
{
    bzero((char*)attrp, sizeof(*attrp));  // TODO: remove, debugging.
    int err = Refresh("Getattr(stat)");
    VLOG(2) << "FuseKvfsInode::Getattr(stat) ino " << attr.st_ino << " err " << err
            << " mode " << std::oct << attr.st_mode;
    if (err == 0) {
        *attrp = attr;
    }
    return err;
}

int FuseKvfsInode::Evict()
{
    Refresh();  // TODO(): cheap, but is refresh necessary here?
    DVLOG(1) << "Evict: " << attr.st_ino << " " << get_name();
    return file.Evict();
}

void FuseKvfsInode::Setattr(fuse_req_t& req, const struct stat& set_attr,
                 int to_set, struct fuse_file_info *fi)
{
    Status status = Status::STATUS_OK;
    pqfs::xid_t xid = kvfs_mount->get_change_log()->NextXid();
    int err = Refresh();
    VLOG(2) << "Setattr: ino " << attr.st_ino
        << " to_set " << to_set << " refresh " << err;
    if (to_set & FUSE_SET_ATTR_SIZE) {
        file.getLogFSMetaData().set_xid(xid);
        status = file.SetSize(set_attr.st_size);
        err = StatusToErrno(status);
        // Not sure if to_set ever has ...SIZE along with other flags.
        VLOG(2) << "Setattr: size " << attr.st_size
            << " SetSize " << set_attr.st_size
            << " status " << status
            << " remaining to_set " << (to_set & ~FUSE_SET_ATTR_SIZE);
        if (attr.st_size != set_attr.st_size) {
            attr.st_size = set_attr.st_size;
            set_state(DIRTY, "SetAttr");
        }
    }
    struct stat new_attr(attr);
    if (err == 0 && (to_set & ~FUSE_SET_ATTR_SIZE)) {
        if (status == Status::STATUS_OK) {
            if (to_set & FUSE_SET_ATTR_MODE) {
                new_attr.st_mode = set_attr.st_mode;
            }
            if (to_set & FUSE_SET_ATTR_UID) {
                new_attr.st_uid = set_attr.st_uid;
            }
            if (to_set & FUSE_SET_ATTR_GID) {
                new_attr.st_gid = set_attr.st_gid;
            }
            if (to_set & FUSE_SET_ATTR_ATIME) {
                new_attr.st_atime = set_attr.st_atime;
            }
            if (to_set & FUSE_SET_ATTR_MTIME) {
                new_attr.st_mtime = set_attr.st_mtime;
            }
            // TODO: Confirm that this is what _NOW means. Docs don't say.
            if (to_set & FUSE_SET_ATTR_ATIME_NOW) {
                new_attr.st_atime = time((time_t*)nullptr);
            }
            if (to_set & FUSE_SET_ATTR_MTIME_NOW) {
                new_attr.st_mtime = time((time_t*)nullptr);
            }
            file.getLogFSMetaData().set_xid(xid);
            file.SetAttr(&new_attr);
            attr = new_attr;
            set_state(CLEAN, "Setattr");  // because Setattr persisted.
        }
    }
    if (err) {
        fuse_reply_err(req, err);
    } else {
        pqfs::proto::ChangeLogEntry entry;
        entry.set_xid(xid);
        entry.set_operation(pqfs::proto::ChangeLogEntry::SETATTR);
        entry.set_fs_id(get_fs_id());
        entry.set_inode_num(attr.st_ino);
        // TODO: what should offsets represent? Part affected? New length?
        //entry.set_begin_offset(0);
        entry.set_end_offset(file.get_size());
        kvfs_mount->get_change_log()->Append(entry);
        fuse_reply_attr(req, &new_attr, kvfs_mount->get_fuse_attr_timeout());
    }
}

void FuseKvfsInode::Lookup(fuse_req_t& req, const char *name)
{
    int err = 0;
    struct fuse_entry_param e = {0};
    e.attr_timeout = kvfs_mount->get_fuse_attr_timeout();
    e.entry_timeout = kvfs_mount->get_fuse_entry_timeout();

    fuse_ino_t child;
    if ((child = file.findHandle(name)) == 0) {
        err = ENOENT;
    }

    if (err == 0) {
        FuseKvfsInode* inodep = kvfs_mount->GetInode(child, true, "Lookup");
        // TODO: Move this block to a common refresh function.
        err = inodep->Refresh();
        if (inodep->state != INVALID) {
            e.ino = (uintptr_t) inodep->attr.st_ino;
            e.attr = inodep->attr;
            inodep->set_state(CLEAN, "Lookup");
        } else {
            EXPECT(err != 0);
        }
    }
    VLOG(2) << "Lookup status " << " name " << name <<  " err " << err;
    if (err) {
        EXPECT(fuse_reply_err(req, err) == 0);
    } else {
        EXPECT(fuse_reply_entry(req, &e) == 0);
    }
}

void FuseKvfsInode::Create(
        fuse_req_t& req, const char *name, mode_t mode,
        struct fuse_file_info *fi)
{
    int err = 0;
    fuse_ino_t child = file.findHandle(name);
    if (child != 0) {
       err = EEXIST;
    } else {
        // Allocate an xid. If we have an error and don't end up adding a log
        // entry, no harm: xid's can be discontiguous.
        pqfs::xid_t xid = kvfs_mount->get_change_log()->NextXid();
        child = kvfs_mount->GetNextIno();
        FuseKvfsInode* new_inodep = kvfs_mount->GetInode(child, true, "Create");
        new_inodep->set_name(name);
        new_inodep->set_mode(mode);
        new_inodep->set_state(INVALID, "Create");
        // TODO: set chunk size from kvfs_mount.
        new_inodep->file.getLogFSMetaData()
            .addPathInfo(attr.st_ino, name);
        new_inodep->file.getLogFSMetaData().set_xid(xid);
        file.getLogFSMetaData().set_xid(xid);  // TODO: what if create fails?
        // TODO: ugh - LogFSFile::create modifies the parent inode object
        // in the KVS.
        err = StatusToErrno(
                new_inodep->file.create(&file));  // parent
        if (err == 0) {
            {
                pqfs::proto::ChangeLogEntry entry;
                entry.set_xid(xid);
                entry.set_operation(pqfs::proto::ChangeLogEntry::CREAT);
                entry.set_fs_id(new_inodep->get_fs_id());
                entry.set_inode_num(child);
                entry.set_parent_inode_num(attr.st_ino);
                entry.set_new_name(name);  // TODO: Not needed for tiering.
                kvfs_mount->get_change_log()->Append(entry);
            }
            fi->fh = reinterpret_cast<uint64_t>(new_inodep);
            fi->keep_cache = kvfs_mount->get_fuse_keep_cache();
            struct fuse_entry_param e = {0};
            e.ino = child;
            EXPECT(new_inodep->Refresh() == 0);
            e.attr = new_inodep->attr;
            e.attr_timeout = kvfs_mount->get_fuse_attr_timeout();
            e.entry_timeout = kvfs_mount->get_fuse_entry_timeout();
            fuse_reply_create(req, &e, fi);
        }
    }
    if (err) {
        fuse_reply_err(req, err);
    }
}

void FuseKvfsInode::Mkdir(fuse_req_t& req, const char *name, mode_t mode)
{
    int err = Refresh();
    fuse_ino_t child = file.findHandle(name);
    if (child != 0) {
       err = EEXIST;
    } else {
        pqfs::xid_t xid = kvfs_mount->get_change_log()->NextXid();
        child = kvfs_mount->GetNextIno();
        FuseKvfsInode* new_inodep = kvfs_mount->GetInode(child, true, "Mkdir");
        new_inodep->set_name(name);
        new_inodep->set_mode(S_IFDIR | mode);
        new_inodep->set_state(INVALID, "Mkdir child");
        // TODO: set chunk size from kvfs_mount.
        new_inodep->file.getLogFSMetaData()
            .addPathInfo(attr.st_ino, name);
        new_inodep->file.getLogFSMetaData().set_xid(xid);
        file.getLogFSMetaData().set_xid(xid);
        // TODO: ugh - LogFSFile::create modifies the parent inode object
        // in the KVS.
        err = StatusToErrno(
                new_inodep->file.create(&file));
        if (err == 0) {
            // The successful create call will have updated file's link count and
            // mod-time. Mark our attrs as invalid to make sure we refresh from
            // underlying file.
            set_state(INVALID, "Mkdir parent");
            {
                pqfs::proto::ChangeLogEntry entry;
                entry.set_xid(xid);
                entry.set_operation(pqfs::proto::ChangeLogEntry::CREAT);
                entry.set_fs_id(new_inodep->get_fs_id());
                entry.set_inode_num(child);
                entry.set_parent_inode_num(attr.st_ino);
                entry.set_new_name(name);  // TODO: Not needed for tiering.
                kvfs_mount->get_change_log()->Append(entry);
            }
            struct fuse_entry_param e = {0};
            e.ino = child;
            EXPECT(new_inodep->Refresh() == 0);
            e.attr = new_inodep->attr;
            e.attr_timeout = kvfs_mount->get_fuse_attr_timeout();
            e.entry_timeout = kvfs_mount->get_fuse_entry_timeout();
            EXPECT(fuse_reply_entry(req, &e) == 0);
        } else {
            new_inodep->Release(1, "Mkdir err");
        }
    }
    if (err) {
        fuse_reply_err(req, err);
    }
}

void FuseKvfsInode::Rmdir(fuse_req_t& req, const char *name)
{
    int err = 0;
    fuse_ino_t child = file.findHandle(name);
    if (child == 0) {
        err = ENOENT;
    } else {
        FuseKvfsInode* child_inodep = kvfs_mount->GetInode(child, true, "Rmdir");
        if (!S_ISDIR(child_inodep->attr.st_mode)) {
            err = ENOTDIR;
        } else {
            pqfs::xid_t xid = kvfs_mount->get_change_log()->NextXid();
            child_inodep->file.getLogFSMetaData().set_xid(xid);
            file.getLogFSMetaData().set_xid(xid);
            err = StatusToErrno(
                    child_inodep->file.remove(&file, name));
            if (err == 0) {
                pqfs::proto::ChangeLogEntry entry;
                entry.set_operation(pqfs::proto::ChangeLogEntry::RMDIR);
                entry.set_fs_id(child_inodep->get_fs_id());
                entry.set_inode_num(child);
                entry.set_parent_inode_num(attr.st_ino);
                entry.set_new_name(name);  // TODO: Not needed for tiering.
                kvfs_mount->get_change_log()->Append(entry);
            }
        }
        child_inodep->Release(1, "Rmdir");
    }
    fuse_reply_err(req, err);
}

void FuseKvfsInode::Unlink(fuse_req_t& req, const char *name)
{
    int err = 0;
    uint64_t child = file.findHandle(name);
    if (child == 0) {
        err = ENOENT;
    } else {
        // TODO: This is broken for files with multiple links or files which may be
        // currently open.
        FuseKvfsInode* inodep = kvfs_mount->GetInode(child, true, "Unlink");
        err = StatusToErrno(inodep->file.remove(&file, name));
        inodep->Release(1, "Unlink");
    }
    if (err == 0) {
        pqfs::proto::ChangeLogEntry entry;
        entry.set_operation(pqfs::proto::ChangeLogEntry::UNLINK);
        entry.set_fs_id(get_fs_id());
        entry.set_inode_num(child);
        entry.set_parent_inode_num(attr.st_ino);
        kvfs_mount->get_change_log()->Append(entry);
    }
    fuse_reply_err(req, err);
}

void FuseKvfsInode::Link(fuse_req_t& req, ino_t ino, const char* name)
{
    int err = Refresh();
    struct fuse_entry_param e = {0};
    e.attr_timeout = kvfs_mount->get_fuse_attr_timeout();
    e.entry_timeout = kvfs_mount->get_fuse_entry_timeout();
    uint64_t child = file.findHandle(name);
    FuseKvfsInode* inodep = nullptr;
    // Does FUSE check for target existence first?
    if (child != 0) {
        err = EEXIST;
    } else {
        inodep = kvfs_mount->GetInode(ino, true, "Link");
        // Since we'll be updating inode's link count and ctime.
        inodep->Refresh();
        err = StatusToErrno(file.link(&inodep->file, name));
        inodep->Release(1, "Link");
        e.ino = (uintptr_t) inodep->attr.st_ino;
        e.attr = inodep->attr;
    }
    if (err == 0) {
        inodep->set_state(INVALID, "Link file"); // nlink & ctime updated
        set_state(INVALID, "Link dir");  // mtime updated
        pqfs::proto::ChangeLogEntry entry;
        entry.set_operation(pqfs::proto::ChangeLogEntry::LINK);
        entry.set_fs_id(get_fs_id());
        entry.set_inode_num(ino);
        entry.set_parent_inode_num(attr.st_ino);
        kvfs_mount->get_change_log()->Append(entry);
    }
    if (err != 0) {
        fuse_reply_err(req, err);
    } else {
        EXPECT(fuse_reply_entry(req, &e) == 0);
    }
}

void FuseKvfsInode::Read(fuse_req_t& req, size_t size, off_t off,
              struct fuse_file_info *fi)
{
    int error = 0;
    size_t actual_len = 0;
    VLOG(2) << "Read ino " << attr.st_ino << " size " << size << " off " << off;
    // TODO: Will reads ever be large, or is this going to be buf-size max?
    // TODO: reduce data copies!
    char* buf = static_cast<char*>(malloc(size));
    if (buf == nullptr) {
        error = ENOMEM;
        goto err_out;
    }
    if ((error = Refresh()) != 0) {
        goto err_out;
    }
    error = StatusToErrno(
            file.read(off, size, buf, &actual_len, true));  // update atime.
    // use fuse_reply_buf for now. May eventually use reply_iov or _data
    // if request is large enough to need multiple segments for data.
    if (error == 0) {
        EXPECT(fuse_reply_buf(req, buf, actual_len) == 0);
    }
err_out:
    if (error) {
        fuse_reply_err(req, error);
        LOG(ERROR) << "Read ino " << attr.st_ino << " error  " << error;
    }
    free(buf);
}

void FuseKvfsInode::Write(fuse_req_t& req, const char *buf,
               size_t size, off_t off, struct fuse_file_info *fi)
{
    VLOG(2) << "Write ino " << attr.st_ino << " size " << size
        << " off " << off;
    size_t actual_len = 0;
    int error = 0;

    Refresh();
    // persist inode after write. TODO: flush on close?
    Status status = file.write(off, size, buf, &actual_len, true);
    if (status == Status::STATUS_OK) {
        attr.st_size = std::max(attr.st_size, (off_t)(off + actual_len));
        attr.st_mtime = time((time_t*)nullptr);
        set_state(DIRTY, "Write");
        //
        pqfs::proto::ChangeLogEntry entry;
        entry.set_operation(pqfs::proto::ChangeLogEntry::COMMIT);
        entry.set_fs_id(get_fs_id());
        entry.set_inode_num(attr.st_ino);
        entry.set_begin_offset(off);
        entry.set_end_offset(off + actual_len);
        kvfs_mount->get_change_log()->Append(entry);
        if (size != actual_len) {
            LOG(ERROR) << "Write: size " << size << " actual " << actual_len;
        }
    } else {
        error = StatusToErrno(status);
    }
    if (error == 0) {
        int fuse_reply_ret = fuse_reply_write(req, actual_len);
        if (fuse_reply_ret != 0) {
            LOG(ERROR) << "Write: fuse_reply_write ret " << fuse_reply_ret;
        }
    } else {
        fuse_reply_err(req, error);
        LOG(ERROR) << "Write ino " << attr.st_ino << " error  " << error;
    }
}


/**
 * Flush method
 *
 * This is called on each close() of the opened file.
 *
 * Since file descriptors can be duplicated (dup, dup2, fork), for
 * one open call there may be many flush calls.
 *
 * Filesystems shouldn't assume that flush will always be called
 * after some writes, or that if will be called at all.
 *
 * fi->fh will contain the value set by the open method, or will
 * be undefined if the open method didn't set any value.
 *
 * NOTE: the name of the method is misleading, since (unlike
 * fsync) the filesystem is not forced to flush pending writes.
 * One reason to flush data, is if the filesystem wants to return
 * write errors.
 *
 * If the filesystem supports file locking operations (setlk,
 * getlk) it should remove all locks belonging to 'fi->owner'.
 *
 * Valid replies:
 *   fuse_reply_err
 *
 * @param req request handle
 * @param ino the inode number
 * @param fi file information
 */
void FuseKvfsInode::Flush(fuse_req_t& req, struct fuse_file_info *fi)
{
    VLOG(2) << "Flush ino " << attr.st_ino;
    // Refresh();  // Shouldn't be necessary?
    EXPECT(state != INVALID);
    int error = StatusToErrno(file.flush());
    fuse_reply_err(req, error);
}

void FuseKvfsInode::Readdir(
        fuse_req_t& req,
        size_t size,
        off_t offset,  // a token ("next offset"), not a byte offset.
        struct fuse_file_info *fi)
{
    char *buf;
    int err = 0;
    VLOG(2) << "Readdir ino " << attr.st_ino;
    buf = (char *) calloc(size, 1);
    if (!buf) {
        err = ENOMEM;
    } else {
        DirEntryAdder dir_entry_adder(kvfs_mount, buf, size);
        err = StatusToErrno(
                file.listDirectory(&dir_entry_adder, offset));
        if (err == 0) {
            VLOG(2) << "Readdir ino " << attr.st_ino << " offset " << offset
                << " size " << dir_entry_adder.get_total_bytes_added();
            if (VLOG_IS_ON(3)) {
                hexdump(buf, dir_entry_adder.get_total_bytes_added(),
                    std::cout);
            }
            fuse_reply_buf(req, buf, dir_entry_adder.get_total_bytes_added());
        }
    }
    if (err != 0) {
         fuse_reply_err(req, err);
    }
    free(buf);
    return;
}

int FuseKvfsInode::Refresh(const char* whence) {
    int err = 0;
    if (state == INVALID) {
        err = StatusToErrno(file.stat(&attr));
        if (err == 0) {
            set_state(CLEAN, "Refresh");  // since we just filled in attr.
        } else {
            LOG(WARNING) << "Refresh(" << whence << ") ino " << file.getHandle()
                << " err " << err;
        }
    }
    return err;
}

void FuseKvfsInode::set_state(State new_state, const char* whence)
{
    VLOG(3) << "set_state i " << attr.st_ino << " (" << whence << ") from " << state << " to " << new_state;
    state = new_state;
}

const uint64_t FuseKvfsInode::get_fs_id() { return kvfs_mount->get_fs_id(); }

