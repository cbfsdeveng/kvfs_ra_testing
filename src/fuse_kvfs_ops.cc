/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/

/*
 * gcc -Wall fuse_lo-plus.c `pkg-config fuse3 --cflags --libs` -o fuse_lo-plus
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse_lowlevel.h>
#include <glog/logging.h>
#include "hexdump.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "fuse_kvfs_ops.h"
#include "kvfs_mount.h"
#include "Status.h"
#include "fuse_err_ops.h"

static KvfsMount* KvfsMountData(fuse_req_t& req)
{
    return static_cast<KvfsMount*>(fuse_req_userdata(req));
}

static
FuseKvfsInode *fk_ino_to_inode(
        fuse_req_t req,
        fuse_ino_t ino,
        const char* whence)
{
    // find KvfsMount from req via fuse_req_user_data;
    KvfsMount* kvfs_mount = KvfsMountData(req);
    FuseKvfsInode* inodep = kvfs_mount->GetInode(ino, true, whence);
    return inodep;
}

// Returns inode pointer, unheld (except by the reference in fi->fh).
// Assumes fi refers to an open file or directory (and hence that fi->fh is our
// stashed inode pointer).
static FuseKvfsInode* fk_fi_to_inode(
        const struct fuse_file_info *fi,
        fuse_ino_t ino)  // For debugging only.
{
    assert(fi != nullptr);
    FuseKvfsInode* inodep = reinterpret_cast<FuseKvfsInode*>(fi->fh);
    EXPECT(inodep != nullptr && inodep->attr.st_ino == ino);
    assert(inodep != nullptr);
    return inodep;
}

static void fk_getattr( fuse_req_t req, fuse_ino_t ino,
        struct fuse_file_info *fi) // @future: currently always NULL
{
    (void) fi;
    VLOG(2) << "fk_getattr ino " << ino;
    FuseKvfsInode* inodep = fk_ino_to_inode(req, ino, "fk_getattr");
    inodep->Getattr(req, fi);
    inodep->Release(1, "fk_getattr");
}

/**
 * Set file attributes
 *
 * In the 'attr' argument only members indicated by the 'to_set'
 * bitmask contain valid values.  Other members contain undefined
 * values.
 *
 * If the setattr was invoked from the ftruncate() system call
 * under Linux kernel versions 2.6.15 or later, the fi->fh will
 * contain the value set by the open method or will be undefined
 * if the open method didn't set any value.  Otherwise (not
 * ftruncate call, or kernel version earlier than 2.6.15) the fi
 * parameter will be NULL.
 *
 * Valid replies:
 *   fuse_reply_attr
 *   fuse_reply_err
 *
 * @param req request handle
 * @param ino the inode number
 * @param attr the attributes
 * @param to_set bit mask of attributes which should be set
 * @param fi file information, or NULL
 *
 * Changed in version 2.5:
 *     file information filled in for ftruncate
 */

static void fk_setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr,
                 int to_set, struct fuse_file_info *fi)
{
    FuseKvfsInode* inodep = fk_ino_to_inode(req, ino, "fk_setattr");
    inodep->Setattr(req, *attr, to_set, fi);
    inodep->Release(1, "fk_setattr");
}

static void fk_lookup(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    VLOG(2) << "fk_lookup parent ino " << parent << " name '" << name << "'";
    FuseKvfsInode* parent_inodep = fk_ino_to_inode(req, parent, "fk_lookup");
    parent_inodep->Lookup(req, name);
    parent_inodep->Release(1, "fk_lookup");
}

static void fk_forget(fuse_req_t req, fuse_ino_t ino, uint64_t nlookup)
{
    FuseKvfsInode *inode = fk_ino_to_inode(req, ino, "fk_forget");

    VLOG(2) << "fk_forget ino " << ino << " nlookup " << nlookup
        << " held " << inode->Hold(0, "");

    if (nlookup > 1000) {
        LOG(ERROR) << "fk_forget: ignore crazy nlookup " << nlookup;
        nlookup = 0;
    }
    if (inode->Release(nlookup + 1, "fk_forget") == 0) {  // + 1 for fk_ino_to_inode hold.
        VLOG(2) << "fk_forget: FreeInode " << ino;
        KvfsMount* kvfs_mount = KvfsMountData(req);
        kvfs_mount->FreeInode(inode);
    }
    fuse_reply_none(req);
}

static void fk_readlink(fuse_req_t req, fuse_ino_t ino)
{
    LOG(ERROR) << "fk_readlink not yet supported";
    return (void) fuse_reply_err(req, ENOTSUP);
}

static void fk_opendir(fuse_req_t req, fuse_ino_t ino,
                       struct fuse_file_info *fi)
{
    FuseKvfsInode* inodep = fk_ino_to_inode(req, ino, "fk_opendir");
    fi->fh = (uint64_t)inodep;
    VLOG(2) << "fk_opendir ino " << ino;
    fuse_reply_open(req, fi);
    // Do not call inodep->Release since fi->fh now references inode.
    return;
}

static void fk_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
                       off_t offset, struct fuse_file_info *fi)
{
    FuseKvfsInode* inodep = fk_fi_to_inode(fi, ino);
    inodep->Readdir(req, size, offset, fi);
}

static void fk_releasedir(fuse_req_t req, fuse_ino_t ino,
                          struct fuse_file_info *fi)
{
    FuseKvfsInode* inodep = fk_fi_to_inode(fi, ino);
    fi->fh = 0;
    inodep->Release(1, "fk_release");
    fuse_reply_err(req, 0);
}

static void fk_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    VLOG(2) << "fk_open ino " << ino;
    FuseKvfsInode* inodep = fk_ino_to_inode(req, ino, "fk_opendir");
    KvfsMount* kvfs_mount = KvfsMountData(req);
    fi->fh = (uint64_t)inodep;
    fi->keep_cache = kvfs_mount->get_fuse_keep_cache();
    fi->direct_io = kvfs_mount->get_fuse_direct_io();
    // TODO: Should reply_open be moved to to an inode-op?
    fuse_reply_open(req, fi);
    // Do not call inodep->Release since fi->fh now references inode.
}

/**
 * Read data
 *
 * Read should send exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the file
 * has been opened in 'direct_io' mode, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * fi->fh will contain the value set by the open method, or will
 * be undefined if the open method didn't set any value.
 *
 * Valid replies:
 *   fuse_reply_buf
 *   fuse_reply_iov
 *   fuse_reply_data
 *   fuse_reply_err
 *
 * @param req request handle
 * @param ino the inode number
 * @param size number of bytes to read
 * @param off offset to read from
 * @param fi file information
 */
void fk_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
              struct fuse_file_info *fi)
{
    FuseKvfsInode* inodep = fk_fi_to_inode(fi, ino);
    inodep->Read(req, size, off, fi);
}

/**
 * Write data
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the file has
 * been opened in 'direct_io' mode, in which case the return value
 * of the write system call will reflect the return value of this
 * operation.
 *
 * fi->fh will contain the value set by the open method, or will
 * be undefined if the open method didn't set any value.
 *
 * Valid replies:
 *   fuse_reply_write
 *   fuse_reply_err
 *
 * @param req request handle
 * @param ino the inode number
 * @param buf data to write
 * @param size number of bytes to write
 * @param off offset to write to
 * @param fi file information
 */
void fk_write(fuse_req_t req, fuse_ino_t ino, const char *buf,
               size_t size, off_t off, struct fuse_file_info *fi)
{
    FuseKvfsInode* inodep = fk_fi_to_inode(fi, ino);
    inodep->Write(req, buf, size, off, fi);
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
void fk_flush(fuse_req_t req, fuse_ino_t ino,
               struct fuse_file_info *fi)
{
    FuseKvfsInode* inodep = fk_fi_to_inode(fi, ino);
    inodep->Flush(req, fi);
}

// Note that here fi will refer to the new file.
static void fk_create(
        fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode,
        struct fuse_file_info *fi)
{
    VLOG(2) << "fk_create parent " << parent << " name " << name << " mode "
        << mode;
    KvfsMount* kvfs_mount = KvfsMountData(req);
    FuseKvfsInode* parent_inodep = kvfs_mount->GetInode(parent, true,
            "fk_create");
    // Create the file and fill in fi->fh, etc.
    parent_inodep->Create(req, name, mode, fi);
    parent_inodep->Release(1, "fk_create");
}

static void fk_mkdir(fuse_req_t req, fuse_ino_t parent, const char *name,
        mode_t mode)
{
    VLOG(2) << "fk_mkdir parent " << parent << " name " << name;
    KvfsMount* kvfs_mount = KvfsMountData(req);
    FuseKvfsInode* parent_inodep = kvfs_mount->GetInode(parent, true, "fk_mkdir");
    parent_inodep->Mkdir(req, name, mode);
    parent_inodep->Release(1, "fk_mkdir");
}

static void fk_rmdir(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    VLOG(2) << "fk_rmdir parent " << parent << " name " << name;
    KvfsMount* kvfs_mount = KvfsMountData(req);
    FuseKvfsInode* parent_inodep = kvfs_mount->GetInode(parent, true, "fk_rmdir");
    parent_inodep->Rmdir(req, name);
    parent_inodep->Release(1, "fk_rmdir");
}

/**
 * Remove a file
 *
 * If the file's inode's lookup count is non-zero, the file
 * system is expected to postpone any removal of the inode
 * until the lookup count reaches zero (see description of the
 * forget function).
 *
 * Valid replies:
 *   fuse_reply_err
 *
 * @param req request handle
 * @param parent inode number of the parent directory
 * @param name to remove
 */
void fk_unlink(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    VLOG(2) << "fk_unlink: parent " << parent << " name " << name;
    KvfsMount* kvfs_mount = KvfsMountData(req);
    FuseKvfsInode* parent_inodep = kvfs_mount->GetInode(parent, true,
            "fk_unlink");
    parent_inodep->Unlink(req, name);
    parent_inodep->Release(1, "fk_unlink");
}


/**
 * Create a hard link
 *
 * Valid replies:
 *   fuse_reply_entry
 *   fuse_reply_err
 *
 * @param req request handle
 * @param ino the old inode number
 * @param newparent inode number of the new parent directory
 * @param newname new name to create
 */
void fk_link(fuse_req_t req, fuse_ino_t ino, fuse_ino_t newparent,
        const char *newname)
{
    VLOG(2) << "fk_link: newparent " << newparent << " newname " << newname;
    KvfsMount* kvfs_mount = KvfsMountData(req);
    FuseKvfsInode* newparent_inodep = kvfs_mount->GetInode(newparent, true,
            "fk_link");
    newparent_inodep->Link(req, ino, newname);
    newparent_inodep->Release(1, "fk_link");
}

#if 0
/** Rename a file
 *
 * If the target exists it should be atomically replaced. If
 * the target's inode's lookup count is non-zero, the file
 * system is expected to postpone any removal of the inode
 * until the lookup count reaches zero (see description of the
 * forget function).
 *
 * Valid replies:
 *   fuse_reply_err
 *
 * @param req request handle
 * @param parent inode number of the old parent directory
 * @param name old name
 * @param newparent inode number of the new parent directory
 * @param newname new name
 */
void fk_rename(fuse_req_t req, fuse_ino_t parent, const char *name,
                fuse_ino_t newparent, const char *newname)
{
    VLOG(2) << "fk_rename: parent " << parent << " name " << name
        << " newparent " << newparent << " newname " << newname;
    KvfsMount* kvfs_mount = KvfsMountData(req);
    FuseKvfsInode* newparent_inodep = kvfs_mount->GetInode(newparent, true,
            "fk_link");
    newparent_inodep->Link(req, ino, newname);
    newparent_inodep->Release(1, "fk_link");
}
#endif

static void fk_mknod(fuse_req_t req, fuse_ino_t parent, const char *name,
        mode_t mode, dev_t rdev)
{
    LOG(ERROR) << "fk_mknod NYI parent " << parent << " name " << name;
    fuse_reply_err(req, ENOTSUP);
}

static void fk_release(fuse_req_t req, fuse_ino_t ino,
                       struct fuse_file_info *fi)
{
    FuseKvfsInode* inodep = fk_fi_to_inode(fi, ino);
    fuse_reply_err(req, 0);
    fi->fh = 0;
    inodep->Release(1, "fk_release");
}

static struct fuse_lowlevel_ops fk_oper;

size_t fk_oper_size() { return sizeof(fuse_lowlevel_ops); }

struct fuse_lowlevel_ops* fk_init_oper()
{
    if (getenv("FUSE_KVFS_ERR_OPS"))
        fk_oper = *err_init_oper();

    fk_oper.create = fk_create;
    fk_oper.forget = fk_forget;
    fk_oper.getattr = fk_getattr;
    fk_oper.setattr = fk_setattr;
    fk_oper.lookup = fk_lookup;
    fk_oper.mkdir = fk_mkdir;
    fk_oper.rmdir = fk_rmdir;
    fk_oper.unlink = fk_unlink;
    fk_oper.link = fk_link;
    // fk_oper.rename = fk_rename;
    fk_oper.mknod = fk_mknod;
    fk_oper.opendir = fk_opendir;
    fk_oper.open = fk_open;
    fk_oper.read = fk_read;
    fk_oper.write = fk_write;
    fk_oper.flush = fk_flush;
    fk_oper.readdir = fk_readdir;
    fk_oper.readlink = fk_readlink;
    fk_oper.releasedir = fk_releasedir;
    fk_oper.release = fk_release;

    return &fk_oper;
}
