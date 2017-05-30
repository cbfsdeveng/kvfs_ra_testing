/*
 * fuse filesystem that logs and returns errors for all fuse_lowlevel ops.
 * Not intended for actual mounting, but can be used to initialize all ops for
 * other filesystems while in development.
 */

#include <fuse_lowlevel.h>

size_t err_oper_size();

struct fuse_lowlevel_ops* err_init_oper() ;
