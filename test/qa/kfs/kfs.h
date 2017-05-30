/*
 * common definitions
 *
 * $Id$
 */


typedef unsigned int u32_t;
typedef unsigned long long u64_t;

#include <sys/types.h>
#include <sys/stat.h>
/*
 * forward declarations
 */
typedef struct _dir_	dir_t;
typedef struct _info_	info_t;
typedef struct _file_	file_t;
typedef struct _region_	region_t;
/*
 * directory structure - we keep track of parents, siblings and children
 * fd of -1 indicates that we don't have an fd.
 * lists are sorted on name.
 */
struct _dir_ {
  dir_t	    *next, *parent;
  char	    *name;
  int	    fd;
  u32_t	    ndirs, nfiles;
  dir_t	    *d_list;
  file_t    *f_list;
};
/*
 * record of surviving regions in a node, sorted by start address.
 * given that we are clipping them, should not be any overlap.
 * len < 0 indicates async - KSS we don't handle that yet...
 */
struct _region_ {
  region_t  *next;
  u32_t  data;
  int32_t   off, len;
};
/*
 * reference counted file attributes and history.
 * regions are time ordered
 */
struct _info_ {
  int		count;		/* this also serves as link count for us */
  u32_t		nregions;
  int32_t	size;		/* what is *our* idea of the size right now? */
  region_t	*r_list;
  struct stat	stat;
};
/*
 * everything else - hard links have a pointer to an info structure,
 * symlinks don't.
 */
struct _file_ {
  file_t    *next;
  dir_t	    *parent;
  char	    *name;
  int	    fd;
  info_t    *info;
};

/*
 * global variables
 */
extern dir_t *top;
extern char *prefix;
extern int verbose, dryrun;
extern int ndirs, nfiles, nsyms, fd_lo, fd_hi, fd_open;
extern int min_files, max_files;
extern int chunk, size;
extern int do_stop;
extern int bonus;

/*
 * operations 
 */
enum {
/*
 * directory operations.  rmdir happens only on an empty, rmdir_r does
 * a recursive remove.
 */
  OP_MKDIR, OP_RMDIR, OP_RMDIR_R,
/*
 * rename gets its own special ledge in hell.
 */
  OP_RENAME,
/*
 * node create/delete ops
 */
  OP_CRE, OP_LINK, OP_SYM, OP_DEL,
/*
 * I/O or file length affected
 */
  OP_SYNC, OP_ASYNC, OP_TRUNC,
/*
 * how many ops are there??
 */
  N_OPS
} op_type;
/*
 * ops count and distribution
 */
extern int nops, op, progress, do_time;
extern int op_dist[N_OPS];

/*
 * global functions
 */
/*
 * pick a directory (from the top) and a file (from the top)
 */
extern dir_t	*pick_dir(dir_t *dp, int weight);
extern file_t	*pick_file(dir_t *dp);
extern char	*make_name(dir_t *dir, char *name);


/*
 * needed for verify, fd and pwd setup for dir/files
 * fill in fd and return 0, or return errno.
 */
extern int  fcd(dir_t *dp);
extern int  make_open(file_t *fp, int flags);
extern int  my_close(int fd);
extern int  get_dir_r(dir_t *dirp);
extern int  do_verify(dir_t *exp, dir_t *obs);
extern void print_tree(dir_t *dp);

/*
 * linkage operations - the del side is also responsible for:
 * 1) ndirs, nfiles
 * 2) closing open fds (and fd_open)
 */
void	add_dir(dir_t *parent, dir_t *child), del_dir(dir_t *, dir_t *);
void	add_file(dir_t *, file_t *), del_file(dir_t *, file_t *);
void    move_file(dir_t *from, dir_t *to, file_t *child);
/*
 * update the regions recorded
 */
void	add_write(info_t *ip, u32_t data, u32_t off, int len);
void	add_trunc(info_t *ip, u32_t len);
/*
 * operations
 */
extern int  do_mkdir(dir_t *);
extern int  do_rmdir(dir_t *);
extern int  do_rmdir_r(dir_t *);
extern int  do_rename(file_t *, dir_t *);

extern int  do_cre(dir_t *);
extern int  do_link(dir_t *, file_t *);
extern int  do_sym(dir_t *, file_t *);
extern int  do_del(file_t *);

extern int  do_sync(file_t *, u32_t data, u32_t off, int len);
extern int  do_async(file_t *, u32_t data, u32_t off, int len);
extern int  do_trunc(file_t *, u32_t len);

