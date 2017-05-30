
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fuse_lowlevel.h>
#include <sys/stat.h>
#include "kvfs_mount.h"


extern struct fuse_lowlevel_ops* fk_init_oper();
extern size_t fk_oper_size();

extern void fk_free(struct FuseKvfsInode *inode);
