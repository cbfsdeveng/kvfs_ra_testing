/*
 * fuse filesystem that logs and returns errors for all fuse_lowlevel ops.
 * Not intended for actual mounting, but can be used to initialize all ops for
 * other filesystems while in development.
 */

#include <errno.h>
#include <fuse_lowlevel.h>
#include <glog/logging.h>

#include "fuse_err_ops.h"


void err_init(void *userdata, struct fuse_conn_info *conn) {
    LOG(ERROR) << "err_init: NYI";
}

void err_destroy(void *userdata)
{
    LOG(ERROR) << "err_destroy : NYI";
}


#define ERR_OP(error, func, args) \
    void err_ ## func args \
    { LOG(ERROR) << "err_"  #func  " : NYI" ; fuse_reply_err(req, error); }

ERR_OP(ENOSYS, lookup, (fuse_req_t req, fuse_ino_t parent, const char *name));
ERR_OP(ENOSYS, forget, (fuse_req_t req, fuse_ino_t ino, unsigned long nlookup));
ERR_OP(ENOSYS, getattr, (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi));
ERR_OP(ENOSYS, setattr, (fuse_req_t req, fuse_ino_t ino, struct stat *attr, int to_set, struct fuse_file_info *fi));
ERR_OP(ENOSYS, readlink, (fuse_req_t req, fuse_ino_t ino));
ERR_OP(ENOSYS, mknod, (fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode, dev_t rdev));
ERR_OP(ENOSYS, mkdir, (fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode));
ERR_OP(ENOSYS, unlink, (fuse_req_t req, fuse_ino_t parent, const char *name));
ERR_OP(ENOSYS, rmdir, (fuse_req_t req, fuse_ino_t parent, const char *name));
ERR_OP(ENOSYS, symlink, (fuse_req_t req, const char *link, fuse_ino_t parent, const char *name));
ERR_OP(ENOSYS, rename, (fuse_req_t req, fuse_ino_t parent, const char *name, fuse_ino_t newparent, const char *newname));
ERR_OP(ENOSYS, link, (fuse_req_t req, fuse_ino_t ino, fuse_ino_t newparent, const char *newname));
ERR_OP(ENOSYS, open, (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi));
ERR_OP(ENOSYS, read, (fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi));
ERR_OP(ENOSYS, write, (fuse_req_t req, fuse_ino_t ino, const char *buf, size_t size, off_t off, struct fuse_file_info *fi));
ERR_OP(ENOSYS, flush, (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi));
ERR_OP(0, release, (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi));
ERR_OP(ENOSYS, fsync, (fuse_req_t req, fuse_ino_t ino, int datasync, struct fuse_file_info *fi));
ERR_OP(ENOSYS, opendir, (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi));
ERR_OP(ENOSYS, readdir, (fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi));
ERR_OP(ENOSYS, readdirplus, (fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi));
ERR_OP(0, releasedir, (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi));
ERR_OP(ENOSYS, fsyncdir, (fuse_req_t req, fuse_ino_t ino, int datasync, struct fuse_file_info *fi));
ERR_OP(ENOSYS, statfs, (fuse_req_t req, fuse_ino_t ino));
ERR_OP(ENOSYS, setxattr, (fuse_req_t req, fuse_ino_t ino, const char *name, const char *value, size_t size, int flags));
ERR_OP(ENOSYS, getxattr, (fuse_req_t req, fuse_ino_t ino, const char *name, size_t size));
ERR_OP(ENOSYS, listxattr, (fuse_req_t req, fuse_ino_t ino, size_t size));
ERR_OP(ENOSYS, removexattr, (fuse_req_t req, fuse_ino_t ino, const char *name));
ERR_OP(ENOSYS, access, (fuse_req_t req, fuse_ino_t ino, int mask));
ERR_OP(ENOSYS, create, (fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode, struct fuse_file_info *fi));
ERR_OP(ENOSYS, getlk, (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi, struct flock *lock));
ERR_OP(ENOSYS, setlk, (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi, struct flock *lock, int sleep));
ERR_OP(ENOSYS, bmap, (fuse_req_t req, fuse_ino_t ino, size_t blocksize, uint64_t idx));
ERR_OP(ENOSYS, ioctl, (fuse_req_t req, fuse_ino_t ino, int cmd, void *arg, struct fuse_file_info *fi, unsigned flags, const void *in_buf, size_t in_bufsz, size_t out_bufsz));
ERR_OP(ENOSYS, poll, (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi, struct fuse_pollhandle *ph));
ERR_OP(ENOSYS, write_buf, (fuse_req_t req, fuse_ino_t ino, struct fuse_bufvec *bufv, off_t off, struct fuse_file_info *fi));
ERR_OP(ENOSYS, retrieve_reply, (fuse_req_t req, void *cookie, fuse_ino_t ino, off_t offset, struct fuse_bufvec *bufv));
ERR_OP(ENOSYS, forget_multi, (fuse_req_t req, size_t count, struct fuse_forget_data *forgets));
ERR_OP(ENOSYS, flock, (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi, int op));
ERR_OP(ENOSYS, fallocate, (fuse_req_t req, fuse_ino_t ino, int mode, off_t offset, off_t length, struct fuse_file_info *fi));

struct fuse_lowlevel_ops err_lowlevel_ops;

size_t err_oper_size() { return sizeof(fuse_lowlevel_ops); }

struct fuse_lowlevel_ops* err_init_oper()
{
struct fuse_lowlevel_ops& ops(err_lowlevel_ops);

//ops.init = err_init;
ops.destroy = err_destroy;
ops.lookup = err_lookup;
ops.forget = err_forget;
ops.getattr = err_getattr;
ops.setattr = err_setattr;
ops.readlink = err_readlink;
ops.mknod = err_mknod;
ops.mkdir = err_mkdir;
ops.unlink = err_unlink;
ops.rmdir = err_rmdir;
ops.symlink = err_symlink;
ops.rename = err_rename;
ops.link = err_link;
ops.open = err_open;
ops.read = err_read;
ops.write = err_write;
ops.flush = err_flush;
ops.release = err_release;
ops.fsync = err_fsync;
ops.opendir = err_opendir;
ops.readdir = err_readdir;
ops.releasedir = err_releasedir;
ops.fsyncdir = err_fsyncdir;
ops.statfs = err_statfs;
ops.setxattr = err_setxattr;
ops.getxattr = err_getxattr;
ops.listxattr = err_listxattr;
ops.removexattr = err_removexattr;
ops.access = err_access;
ops.create = err_create;
ops.getlk = err_getlk;
ops.setlk = err_setlk;
ops.bmap = err_bmap;
ops.ioctl = err_ioctl;
ops.poll = err_poll;
// ops.write_buf = err_write_buf;
//ops.retrieve_reply = err_retrieve_reply;
//ops.forget_multi = err_forget_multi;
ops.flock = err_flock;
ops.fallocate = err_fallocate;
return &ops;
}
