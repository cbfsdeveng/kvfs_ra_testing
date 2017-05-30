/*
 * kill fs test
 *
 * $Id$
 */

#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <dirent.h>
#include <signal.h>

#include <sys/time.h>
#include <sys/resource.h>

#include <string.h>
#include <strings.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "kfs.h"

static void
usage(void)
{
  printf("Usage:\n"
"\t[-v<erbose>]\n"
"\t[-n<ops> #]\n"
"\t[-t<ime> #=120]\n"
"\t[-b<onus> #]\n"
"\t[-s<eed> #]\n"
"\t[-c<hunk> #]\n"
"\t[-l<en> #]\n"
"\t[-f<dopen> lo,hi]\n"
"\t[-F<ilelimit> min,max]\n"
"\t[-D<ryrun>]\n"
"\t[-V<erify>]\n"
"\t[-S<yncflag>]\n"
"\t[-m<etatest>]\n"
"\t[-M<onster>]\n"
"\t[-r<and> mkdir,rmdir,rmdir_r,rename cre,link,sym,del sync,async,trunc]\n"
"\t[-p<rogress> #]\n"
"\t[-P<refix> name]\n"
"\t\tdir\n");
  exit(1);
}
/*
 * top level directory in which we test - must exist.
 */
dir_t *top;
/*
 * prefix so we can run multiple copies in the same dir tree
 */
char *prefix;
/*
 * total files and dirs, and open and allowed open file desciptors
 * max I/O size and file size;
 */
int verbose, dryrun, verify;
int ndirs = 0, nfiles = 0, nsyms = 0, fd_lo = 0, fd_hi = 0, fd_open = 0;
int min_files = 10, max_files = 100;
int chunk = 16*1024, size = 128*1024;
int do_stop = 0;

static int do_fsync = 0;
/*
 * chunk to do I/O from
 */
static u32_t *io_chunk;
/*
 * how many did we do, how many were asked of us, how often should
 * we poke our head up to tell the user about progress???
 */
int nops, op, progress, do_time = 120;
/*
 * are we looking for bonus operations??
 */
int bonus;
/*
 * what is the distribution of a ... in the total
 */
int op_dist[N_OPS] = {
/*
 * mkdir, rmdir, rmdir_r, rename
 */
  10, 10, 1, 10,
/*
 * cre, link, sym, del
 */
  15, 5, 5, 5,
/*
 * sync, async, trunc
 */
  1000, 0, 100
};

/*
 * no removes, and a mix of metadata and I/O
 */
int op_monster[N_OPS] = { 10,0,0,10, 15,5,5,0, 100, 0, 1 };

void
sig_int(int arg)
{
  do_stop = 1;
}
/*
 * we have to make sure file descriptor behavior is the same on
 * dry runs too (for reaping in particular).  We also keep a
 * pseudo inode number for mkdir/create/link/symlink on dryruns.
 */

#define BPM 32

u32_t *fd_bits;
static void reap(void);

int
my_open(char *name, int flags, int mode)
{
  u32_t *bits;
  int i;
  int fd, my_fd, reap_try = 10;

/*
 * first find the fake one
 */
  while(reap_try-- > 0) {
    for(i = fd_hi/BPM, bits = fd_bits ; i-- ; bits++)
      if (*bits)
	break;

    if (i < 0) {
      printf("overallocated open %d lo %d hi %d\n",
	  fd_open, fd_lo, fd_hi);
      reap();
      
    } else {
      break;
    }
  }
  if (reap_try < 0) {
    printf("couldn't cure overallocation\n");
    exit(1);
  }

  i = ffs(*bits) - 1;
  my_fd = (bits - fd_bits)*BPM + i;
  *bits &= ~(1<<i);
/*
 * and see if we are supposed to do a real one
 */
  if (!dryrun) {

    if (do_fsync)
      flags |= O_FSYNC;

    if ((fd = open(name, flags, mode)) < 0)
      return(fd);
    if (fd != my_fd) {
      fprintf(stderr, "my_open: wanted %d got %d\n", my_fd, fd);
      exit(1);
    }
  }

  if (verbose)
    printf("open: %s %d\n", name, my_fd);

  ++fd_open;
  return(my_fd);
}

int
my_close(int fd)
{
  u32_t *bits;
/*
 * don't allow 0/1/2 or -1
 */
  if (fd < 3) {
    fprintf(stderr, "my_close: bogus fd %d\n", fd);
    exit(1);
  }
  if (verbose)
    printf("close: %d\n", fd);
  if(!dryrun && close(fd)) return(errno);
/*
 * put it back into the free pool
 */
  bits = fd_bits + (fd/BPM);
  fd = fd % BPM;
  *bits |= (1<<fd);

  --fd_open;
  return(0);
}

/*
 * seed for random sequences so we can reproduce a given run.
 */
int seed = 42;

extern char *optarg;
extern int optind;

int
main(int argc, char **argv)
{
  int i, pop_err = 0, err, n, fd;
  char *dir, *test = "random";
  struct rlimit r;

  int do_random(dir_t *);
  extern void catchme(void);
/*
 * get args
 */
  while((i = getopt(argc, argv, "b:c:f:F:r:l:mMn:p:s:t:P:vDVS")) > 0) {
    switch(i) {
    default: usage(); break;
    case 'f':
      if (2 != sscanf(optarg, "%d,%d", &fd_lo, &fd_hi)) {
	fprintf(stderr, "Need both lo and hi water\n");
	usage();
      }
      break;
    case 'F':
      if (2 != sscanf(optarg, "%d,%d", &min_files, &max_files)) {
	fprintf(stderr, "Need both min and max files\n");
	usage();
      }
      break;
    case 'r':
      test = "random";
      if (N_OPS != (n = sscanf(optarg, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
			       op_dist + OP_MKDIR,
			       op_dist + OP_RMDIR,
			       op_dist + OP_RMDIR_R,
			       op_dist + OP_RENAME,

			       op_dist + OP_CRE,
			       op_dist + OP_LINK,
			       op_dist + OP_SYM,
			       op_dist + OP_DEL,
			      
			       op_dist + OP_SYNC,
			       op_dist + OP_ASYNC,
			       op_dist + OP_TRUNC))) {
	fprintf(stderr, "Only got %d, need all %d\n", n, N_OPS);
	usage();
      }
      break;
    case 'l': size = strtol(optarg, 0, 0); break;
    case 'c': chunk = strtol(optarg, 0, 0); break;
    case 'b': bonus = strtol(optarg, 0, 0); break;
    case 'm': op_dist[OP_SYNC] = 10 ; op_dist[OP_TRUNC] = 1; break;
    case 'M': memcpy(op_dist, op_monster, sizeof(op_monster)); break;
    case 'n': nops = strtol(optarg, 0, 0); do_time = 0; break;
    case 'p': progress = strtol(optarg, 0, 0); break;
    case 's': seed = strtol(optarg, 0, 0); break;
    case 't': nops = 0x7fffffff ; do_time = atoi(optarg); break;
    case 'v': ++verbose; break;
    case 'D': ++dryrun; break;
    case 'V': ++verify ; break;
    case 'S': ++do_fsync; break;
    case 'P': prefix = optarg; break;
    }
  }

  if (optind + 1 != argc) {
    fprintf(stderr, "Must supply a directory name\n");
    usage();
  }
/*
 * bug#2006 - give us a bloody coredump
 */
  r.rlim_cur = r.rlim_max = RLIM_INFINITY;
  setrlimit(RLIMIT_CORE, &r);
/*
 * let's see what the file descriptor limits are
 */
  getrlimit(RLIMIT_NOFILE, &r);
  if (r.rlim_cur < fd_hi || fd_hi == 0) {
    fd_hi = r.rlim_cur;
    fd_lo = fd_hi/2;
#if 0
    printf("Setting fd hi,lo to %d,%d\n", fd_hi, fd_lo);
#endif
  }
/*
 * set up the file descriptor checking. Round up the number of
 * bits to the nearest mask boundary and put in some slop for
 * overallocation. Account for 0/1/2.
 * KSS - looks like we get sloppy execs or something, as other file
 * descriptors have been seen besides 0/1/2.  We'll dup(0) to see
 * what we get and move on from there.  Note that if there is something
 * that *varies* the first FD from run to run we don't have a hope.
 */
  i = fd_hi;
  i = (i + BPM - 1) & ~(BPM-1);
  i += BPM;
  fd_bits = malloc(i/8);
  memset(fd_bits, ~0, i/8);
/*
 * see what the first FD is by dup(0)/close in case we have extras
 */
  i = dup(0);
  close(i);
  fd_bits[0] &= ~((1<<i)-1);
  fd_open = i;
/*
 * sadly, that is not enough - it looks like STAF (at least) leaves
 * a higher file descriptor open.  Nail them all and report any open
 * ones for debug
 */
  for (i = fd_open; i < fd_hi; i++) {
    if (!close(i)) fprintf(stderr, "kfs: fd %d was open on exec - closed\n", i);
  }
/*
 * set up to take ^C and stop nicely
 */
  signal(SIGINT, sig_int);
/*
 * set up to handle SIGBUS/SIGSEGV
 */
  catchme();
/*
 * always set the sequence generator
 */
  srandom(seed);
/*
 * allocate I/O chunk
 */
  chunk = (chunk + 3) & ~3;
  if (!(io_chunk = (u32_t *)malloc(chunk))) {
    fprintf(stderr, "Can't malloc I/O chunk of %d\n", chunk);
    exit(1);
  }
/*
 * let's run (or dryrun) a test
 */
  dir = argv[optind];
/* 
 * let's see about the directory and its contents
 */
  if ((fd = my_open(dir, O_RDONLY, 0)) < 0) {
    fprintf(stderr, "Can't open dir %s (%s)\n", dir, strerror(errno));
    exit(1);
  }
  if (!(top = malloc(sizeof(*top)))) {
    fprintf(stderr, "Can't malloc top\n");
    exit(1);
  }
  bzero(top, sizeof(*top));
  top->fd = fd;
  top->name = strdup(dir);

  if (verbose)
    printf("START %s %s: n %d size %d seed %d\n",
	   dir, test, nops, size, seed);

  if (!strcmp(test, "random"))
    pop_err = do_random(top);
/*
 * tell about having finished...
 */      
  if (verbose)
    printf("%s %s %s: ndirs %d nfiles %d nsyms %d : %d of %d ops err %d\n",
	   pop_err ? "ERROR" : (dryrun ? "DRY" : "DONE"),
	   dir, test, ndirs, nfiles, nsyms, op, nops, pop_err);
/*
 * now see if we want to use the tree to verify
 */
  if (verify) {
    int o_ndirs, o_nfiles, o_nsyms;
    dir_t v, *vrfy = &v;
/*
 * let's find out what's there now by creating another dir tree
 */
    if ((fd = open(dir, O_RDONLY)) < 0) {
      fprintf(stderr, "verify: Can't open dir %s (%s)\n",
	      dir, strerror(errno));
      exit(1);
    }
    bzero(vrfy, sizeof(*vrfy));
    vrfy->fd = fd;
    vrfy->name = strdup(dir);
    
    o_ndirs = ndirs; ndirs = 0;
    o_nfiles = nfiles; nfiles = 0;
    o_nsyms = nsyms ; nsyms = 0;

    if ((err = get_dir_r(vrfy))) {
      fprintf(stderr, "verify: Couldn't get dir %s (%s)\n",
	      dir, strerror(err));
      exit(1);
    }
/*
 * simple check - do we end up with the same number of dirs and files??
 * KSS - go ahead and compare to see *which* files lost.
 */
    i = 0;
    if (o_ndirs != ndirs || o_nfiles != nfiles || o_nsyms != nsyms) {
      ++i;
      printf("verify: count mistmatch dirs %d != %d || files %d != %d || nsyms %d != %d\n",
	     o_ndirs, ndirs, o_nfiles, nfiles, o_nsyms, nsyms);
    }
/*
 * verify namespace - just compare the two trees.  Should we go on??
 */
    if ((err = do_verify(top, vrfy))) {
      printf("VERIFY: FAIL\n");
      if (verbose > 1) {
	printf("EXPECTED\n");
	print_tree(top);
	printf("OBSERVED\n");
	print_tree(vrfy);
      }
      exit(1);
    }
    printf("VERIFY: OK\n");
  }

  exit(pop_err);
}

/*
 * pick a directory from this tree - we weight the current directory
 * heavily to try and flatten it a bit.
 * weight 0 will guarantee a leaf directory
 * weight 1 will be an even roll for for . and all children
 * weight dp->ndir means 50/50 chance of . vs. all children
 * larger weights move the balance towards the top.
 */

dir_t *
pick_dir(dir_t *dp, int weight)
{
  int i;
  dir_t *sp;
/*
 * if no subdirs, it must be us.
 */
  if (dp->ndirs == 0)
    return(dp);
/*
 * roll the dice for this or a child dir based on weight
 * 0..ndirs-1 is a child, >= ndirs is currrent.
 */  
  i = random() % (dp->ndirs + weight);

  if (i >= dp->ndirs) {
    return(dp);
  } else {
    for (sp = dp->d_list; i-- ; sp = sp->next)
      ;
    return(pick_dir(sp,weight));
  }
}

void
print_tree(dir_t *dp)
{
  dir_t *sp;
  file_t *fp;
  info_t *ip;
/*
 * first print out the relevant info for this level
 */
  printf("{ %s: ndirs %d nfiles %d\n",
	 make_name(dp, ""), dp->ndirs, dp->nfiles);
/*
 * files
 */
  if (dp->nfiles) {
    for (fp = dp->f_list; fp; fp = fp->next)
      if ((ip = fp->info))
	printf("F %s: len %d count %d link %d node %d region %d\n",
	       make_name(dp, fp->name), (int)ip->stat.st_size,
	       (int)ip->count, (int)ip->stat.st_nlink, (int)ip->stat.st_ino,
	       (int)ip->nregions);
      else
	printf("S %s\n", make_name(dp, fp->name));
    
    
    printf("\n");
  }
/*
 * dirs
 */
  if (dp->ndirs) {
/*
 * local
 */
    for (sp = dp->d_list; sp; sp = sp->next)
      printf("D %s: ndirs %d nfiles %d\n",
	     make_name(sp, ""), sp->ndirs, sp->nfiles);
    printf("\n");
/*
 * descend
 */
    for (sp = dp->d_list; sp; sp = sp->next)
      if (sp->ndirs || sp->nfiles)
      print_tree(sp);
  }
  printf("}\n");
}
/*
 * pick a file - which also means making sure we pick a directory
 * with files in it.  Can return symlinks, so caller has to check that.
 */
file_t *
pick_file(dir_t *dp)
{
  int i, t;
  dir_t *sp;
  file_t *fp;

  if (nfiles == 0)
    return(0);

  t = op;
  if (t < 100)
    t = 100;

  do {
    sp = pick_dir(dp, 1);
  } while (!sp->nfiles && --t);

  if (!t) {
    printf("PICK FILE search failed\n");
    print_tree(dp);
    exit(1);
  }

  i = random() % sp->nfiles;
  for (fp = sp->f_list; i-- ; fp = fp->next)
    ;
  return(fp);
}

static void
reap(void)
{
  dir_t *dirp, *dp;
  file_t *fp;
  int reap_count;

  if (fd_open < fd_hi)
    return;
/*
 * pick a random directory and hurt it with a 50/50 chance of
 * closing things that are open.
 */
  reap_count = 0;
  while(fd_open > fd_lo) {
    dirp = pick_dir(top, 1);
    for (dp = dirp->d_list; dp ; dp = dp->next) {
      if (dp->fd > 0 && (random() & 0x4)) {
	my_close(dp->fd);
	dp->fd = -1;
	++reap_count;
      }
    }
    for (fp = dirp->f_list; fp ; fp = fp->next) {
      if (fp->fd > 0 && (random() & 0x8)) {
	my_close(fp->fd);
	fp->fd = -1;
	++reap_count;
      }
    }
  }
  if (verbose)
    printf("reap %d to %d\n", reap_count, fd_open);
}
/*
 * generate a unique name - dryruns can't depend on stat() to tell them
 * about names...
 * also can't do stats, but we can do a pseudo inode number too that we
 * can use on new nodes. Mostly to track writes/truncs to common files
 * in identifying bonus operations.
 */

static u32_t dryrun_ino = 0x10000000;

char *
new_name(void)
{
  static char fname[128];


  sprintf(fname, "%s%08x", prefix ? prefix : "", ++dryrun_ino);

  return(strdup(fname));
}
/*
 * make sure a directory has an fd, and fchdir() to it
 */
int
fcd(dir_t *dp)
{
  int err;
  char *name;

  if (dp->fd == -1) {
    reap();
    name = make_name(dp, "");
    
    if ((dp->fd = my_open(name, O_RDONLY, 0)) < 0) {
      err = errno;
      fprintf(stderr, "fcd: open %s failed (%s)\n",
	      name, strerror(err));
      return(err);
    }
  }
  if (!dryrun && fchdir(dp->fd)) {
    err = errno;
    name = make_name(dp, "");
    fprintf(stderr, "fcd: fchdir %s failed (%s)\n",
	    name, strerror(err));
    return(err);
  }
  return(0);
}
/*
 * make sure file has an fd.  open relative to parent if possible
 */
int
make_open(file_t *fp, int flags)
{
  int err;
  char *name;
/*
 * already open - just return
 * otherwise, reap if necessary
 */
  if (fp->fd != -1)
    return(0);
  
  reap();
/*
 * if parent has an fd, do a relative open, otherwise absolute.
 */
  if (fp->parent->fd != -1) {
    if ((err = fcd(fp->parent)))
      return(err);
    else
      name = fp->name;
  } else {
    name = make_name(fp->parent, fp->name);
  }
/*
 * now open the file
 */
  if ((fp->fd = my_open(name, flags, 0)) < 0) {
    err = errno;
    fprintf(stderr, "make_open: open on %s failed (%s)\n",
	    make_name(fp->parent, fp->name), strerror(err));
    return(err);
  }

  return(0);
}


/*
 * given a directory, make another directory in it
 */

int
do_mkdir(dir_t *dirp)
{
  int err;
  char *name;
  dir_t	*dp;
/*
 * 1) allocate structures
 * 2) if not a dry run, setup parent/child dirs
 * 3) add to the world
 */

  if (!(dp = malloc(sizeof(*dp))) || !(name = new_name()))
    return(ENOMEM);
  else
    bzero(dp, sizeof(*dp));

  if (verbose)
    printf("mkdir: %s\n", make_name(dirp, name));
/*
 * if we're not dry running, then make sure we have a parent set up and
 * do the mkdir()
 */
  if ((err = fcd(dirp)))
    return(err);

  if (!dryrun && mkdir(name, 0755)) {
    err = errno;
    name = make_name(dirp, name);
    fprintf(stderr, "do_mkdir: mkdir %s failed (%s)\n", name, strerror(err));
    return(err);
  }
/*
 * set it up and add it to the tree
 */
  dp->name = name;
  dp->fd = -1;

  add_dir(dirp, dp);

  return(0);
}
/*
 * remove a directory if it's empty (odds are it's not...)
 */
int
do_rmdir(dir_t *dirp)
{
  int err, fail = 0;
  char *name;
/*
 * don't do the top level dir
 */
  if (dirp == top)
    return(0);
/*
 * and expect rmdirs on non-empty directories to fail
 * KSS get more sophisticated about NFS...
 */
  if (dirp->ndirs != 0 || dirp->nfiles != 0)
    fail = 1;

  if (verbose)
    printf("rmdir%c: %s\n", fail ? '-' : '+', make_name(dirp, ""));

/*
 * try for a relative lookup, absolute otherwise
 */
  if (dirp->parent->fd != -1) {
    if ((err = fcd(dirp->parent)))
      return(err);
    else
      name = dirp->name;
  } else {
    name = make_name(dirp, "");
  }

/*
 * if fail wasn't set and we get ENOTEMTPY, then we're probably running
 * into the NFS async sillyREMOVE bug, so retry that case specially.
 */
  if (!dryrun && rmdir(name) && (!fail || errno != ENOTEMPTY)) {
    if (errno == ENOTEMPTY) {
      int   i;

      for (i = 0; i < 10; i++) {
	sleep(1);
	errno = 0;				  /* success doesn't clear */
	if (!rmdir(name) || (errno != ENOTEMPTY)) break;
      }
    }
    if (errno) {
      err = errno;
      fprintf(stderr, "do_rmdir: rmdir of %s failed (%s)\n",
	      make_name(dirp, ""), strerror(err));
      return(err);
    }
  }

  if (!fail)
    del_dir(dirp->parent, dirp);

  return(0);
}

/*
 * do a big hairy remove starting at this dir
 * We honor the operations for a dry run by checking on each operation.
 */
int
do_rmdir_r(dir_t *dirp)
{
  int err;
  dir_t *dp;
  file_t *fp;

  if (dirp == top)
    return(0);
  if (verbose)
    printf("rmdir_r: %s\n", make_name(dirp, ""));
/*
 * first descend into and then remove any subdirs
 */
  while(op < nops && (dp = dirp->d_list)) {
    if ((err = do_rmdir_r(dp)))
      return(err);
  }
/*
 * then remove any files
 */
  while(op < nops && (fp = dirp->f_list)) {
    if (verbose)
      printf("%d: roll -1 nf %d nd %d\n", op, nfiles, ndirs);
    if ((err = do_del(fp)))
      return(err);
    else
      ++op;
  }
/*
 * then lose ourselves
 */
  
  if (op < nops) {
    if (verbose)
      printf("%d: roll -1 nf %d nd %d\n", op, nfiles, ndirs);
    if (!(err = do_rmdir(dirp)))
      ++op;
    return(err);
  } else {
    return(0);
  }
}

/*
 * rename a file/sym - even if we are given the same
 * parent, we change the name to protect the innocent
 */
int
do_rename(file_t *fp, dir_t *dirp)
{
  int err;
  char *old, *new;

  if (!(new = new_name()))
    return(ENOMEM);

  if (verbose) {
    printf("rename: %s -> ", make_name(fp->parent, fp->name));
    printf("%s\n", make_name(dirp, new));
  }
/*
 * if we're not dryrunning, do the deed.
 */
  if ((err = fcd(dirp)))
    return(err);

  old = make_name(fp->parent, fp->name);
  if (!dryrun && rename(old, new)) {
    err = errno;
    fprintf(stderr, "rename: %s -> ", make_name(fp->parent, fp->name));
    fprintf(stderr, "%s failed (%s)\n",
	    make_name(dirp, new), strerror(err));
    return(err);
  }
/*
 * change the name and move the entry - the node number doesn't change
 */
  free(fp->name);
  fp->name = new;
  move_file(fp->parent, dirp, fp);
  return(0);
}
/*
 * create a file in a directory
 */
int
do_cre(dir_t *dirp)
{
  int err;
  char *name;
  file_t *fp;
  info_t *ip;

  if (!(fp = malloc(sizeof(*fp))) ||
      !(ip = malloc(sizeof(*ip))) ||
      !(name = new_name()))
    return(ENOMEM);


  bzero(fp, sizeof(*fp));
  bzero(ip, sizeof(*ip));

  if (verbose)
    printf("create: %s\n", make_name(dirp, name));
/*
 * dryrun is noticed below us
 */
  reap();
  if ((err = fcd(dirp)))
    return(err);
  
  if ((fp->fd = my_open(name, O_RDWR|O_CREAT|O_EXCL, 0644)) < 0) {
    err = errno;
    name = make_name(dirp, name);
    fprintf(stderr, "do_cre: creat %s failed (%s)\n", name, strerror(err));
    return(err);
  }
  ip->size = 0;
  if (dryrun) {
    ip->stat.st_size = 0;
    ip->stat.st_ino = dryrun_ino;
  } else {
    fstat(fp->fd, &ip->stat);
  }

  fp->name = name;
  fp->info = ip;
  add_file(dirp, fp);

  return(0);
}

/*
 * make a hard link
 */
int
do_link(dir_t *dirp, file_t *fp)
{
  int err;
  char *name, *old;
  file_t *nfp;
  info_t *ip = fp->info;

  if (!(nfp = malloc(sizeof(*nfp))) || !(name = new_name()))
    return(ENOMEM);
  else
    bzero(nfp, sizeof(*nfp));

  if (verbose) {
    printf("link: %s -> ", make_name(dirp, name));
    printf("%s\n", make_name(fp->parent, fp->name));
  }
/*
 * if we're not dry running, make sure the parent is setup, then
 * go do the link.
 */
  if ((err = fcd(dirp)))
    return(err);

  old = make_name(fp->parent, fp->name);
  if (!dryrun && link(old, name)) {
    err = errno;
    name = make_name(dirp, name);
    fprintf(stderr, "do_link: link %s -> ", name);
    fprintf(stderr, "%s failed (%s)\n", make_name(fp->parent, fp->name),
	    strerror(err));
    return(err);
  }
/*
 * on a dryrun, we link to the same info, so node number stays the same too
 */
  if (!dryrun) {
    lstat(name, &ip->stat);
    if (ip->stat.st_size != ip->size) {
      fprintf(stderr, "do_link: stat size exp %ld obs %ld\n", 
        	(long)ip->size, (long)ip->stat.st_size);
      err = ERANGE;
    }
  }


  nfp->name = name;
  nfp->info = ip;
  nfp->fd = -1;
  add_file(dirp, nfp);

  return(err);
}

int
do_sym(dir_t *dirp, file_t *fp)
{
  int err;
  char *name, *old;
  file_t *nfp;

  if (!(nfp = malloc(sizeof(*nfp))) || !(name = new_name()))
    return(ENOMEM);
  else
    bzero(nfp, sizeof(*nfp));

  if (verbose) {
    printf("sym: %s -> ", make_name(dirp, name));
    printf("%s\n", make_name(fp->parent, fp->name));
  }
/*
 * if we're not dry running, make sure the parent is setup, then
 * go do the link.
 */
  if ((err = fcd(dirp)))
    return(err);
  
  old = make_name(fp->parent, fp->name);
  if (!dryrun && symlink(old, name)) {
    err = errno;
    name = make_name(dirp, name);
    fprintf(stderr, "do_sym: link %s -> ", name);
    fprintf(stderr, "%s failed (%s)\n", make_name(fp->parent, fp->name),
	    strerror(err));
    
    return(err);
  }
/*
 * symlinks don't have an info, so there is no "node number" to preserve
 */
  nfp->name = name;
  nfp->fd = -1;
  add_file(dirp, nfp);

  return(0);
}

/*
 * delete a link/symlink
 */
int
do_del(file_t *fp)
{
  int err;
  dir_t *dirp = fp->parent;
  char *name;

  if (verbose)
    printf("del: %s %d\n", make_name(fp->parent, fp->name), fp->fd);
/*
 * absolute if the parent isn't open, relative otherwise
 */
  if (fp->parent->fd != -1) {
    if ((err = fcd(fp->parent)))
      return(err);
    else
      name = fp->name;
  } else {
    name = make_name(dirp, fp->name);
  }
  if (!dryrun && unlink(name)) {
    err = errno;
    fprintf(stderr, "do_del: unlink of %s failed (%s)\n",
	    make_name(fp->parent, fp->name), strerror(err));
    return(err);
  }

  del_file(fp->parent, fp);

  return(0);
}
/*
 * write a chunk to a file. len < 0 indicates async.  If we are looking
 * for bonus operations, we record the last write and truncate info so
 * we can see if we got *part* of the last failed operation successfully.
 */

u32_t	lw_ino, lw_patt, lw_off, lw_len;

int
do_write(file_t *fp, u32_t data, u32_t off, int olen)
{
  int i, err;
  char *name;
  u32_t *iop;
  int len;
  int sync;
/*
 * make data a bit different to see about leaking writes and
 * whether it is our data or not
 */
  data |= 0xf0000000;
  data &= 0xfffffff0;
/*
 * make sure we have a file/link
 */
  if (!fp->info)
    return(0);
/*
 * sync writes are marked as < 0, so convert if need be
 */
  len = olen;
  if (len < 0) {
    len = - len;
    sync = 0;
  } else {
    sync = 1;
  }
  if (verbose)
    printf("%s: %s 0x%08x @ %x-%x (%x)\n", sync ? "sync" : "async",
	   make_name(fp->parent, fp->name),
	   data, off, off+len, len);
/*
 * make sure we have an fd
 */
  if ((err = make_open(fp, O_RDWR)))
    return(err);
/*
 * condition off/len to be 4 byte aligned and fill buffer with data
 */
  off = off & ~3;
  len = len & ~3;
  i = len/4;
  iop = io_chunk;
  while(i--)
    *iop++ = data;
/*
 * BSD 2.2.5 doesn't have pwrite...
 */
  i = -2;
  if (!dryrun) {
    if (lseek(fp->fd, (off_t)off, SEEK_SET) == -1 ||
	(i = write(fp->fd, io_chunk, len)) != len) {
      err = errno;
      name = make_name(fp->parent, fp->name);
      fprintf(stderr, "do_write: seek/write of +%x,%x to %s got %x (%s)\n",
	      off, olen, name, i, strerror(err));
      if (!err)
	err = ENOSPC;
      return(err);
    }
    if(sync && fsync(fp->fd)) {
      err = errno;
      fprintf(stderr, "do_write: fsync on %s failed (%s)\n",
	      make_name(fp->parent, fp->name), strerror(err));
      return(err);
    }
    if (off + len > fp->info->size) fp->info->size = off + len;
    fstat(fp->fd, &fp->info->stat);
    if (fp->info->size != fp->info->stat.st_size) {
      fprintf(stderr, "do_write: stat size exp %ld obs %ld\n",
                 (long)fp->info->size, (long)fp->info->stat.st_size);
      return(ERANGE);
    }
  } else {
    if (off + len > fp->info->stat.st_size) {
      fp->info->size = fp->info->stat.st_size = off + len;
    }
  }
/*
 * record the write on the node
 */
  add_write(fp->info, data, off, olen);
/*
 * and if we are doing bonus, record it globally
 */
  if (bonus) {
    lw_ino = fp->info->stat.st_ino;
    lw_patt = data;
    lw_off = off;
    lw_len = len;
  }

  return(0);
}
/*
 * sync/async writes - in case we want to do this with O_SYNC
 */
int
do_sync(file_t *fp, u32_t data, u32_t off, int len)
{
  return(do_write(fp, data, off, len));
}

int
do_async(file_t *fp, u32_t data, u32_t off, int len)
{
  return(do_write(fp, data, off, -len));
}
/*
 * we also try to find bonus truncates
 */

u32_t lt_ino, lt_len;

int
do_trunc(file_t *fp, u32_t len)
{
  int err;
  char *name;
  info_t *ip;
  region_t *rp;

/*
 * we only do real files
 */
  if (!fp->info)
    return(0);
  else
    ip = fp->info;
/*
 * ... at 4 byte boundaries
 */
  len &= ~3;

  if (verbose)
    printf("trunc: %s %x\n", make_name(fp->parent, fp->name), len);
/*
 * allocate a region to indicate the truncate
 */
  if (!(rp = malloc(sizeof(*rp))))
    return(ENOMEM);
  else
    bzero(rp, sizeof(*rp));
/*
 * see if we have an fd for it, otherwise see about relative/absolute
 */
  if (fp->fd != -1) {
    if (!dryrun && ftruncate(fp->fd, (off_t)len)) {
      err = errno;
      fprintf(stderr, "do_trunc: ftruncate on %s to %x failed (%s)\n",
	      make_name(fp->parent, fp->name), len, strerror(err));
      return(err);
    }
    ip->size = len;
    if (!dryrun)
      fstat(fp->fd, &ip->stat);
  } else {
    if (fp->parent->fd != -1) {
      if ((err = fcd(fp->parent)))
	return(err);
      else
	name = fp->name;
    } else {
      name = make_name(fp->parent, fp->name);
    }

    if (!dryrun && truncate(name, (off_t)len)) {
      err = errno;
      fprintf(stderr, "do_trunc: truncate on %s to %x failed (%s)\n",
	      make_name(fp->parent, fp->name), len, strerror(err));
      return(err);
    }
    ip->size = len;
    if (!dryrun)
      stat(name, &ip->stat);
  }

  if (dryrun)
    ip->stat.st_size = len;

  if (ip->size != ip->stat.st_size) {
      fprintf(stderr, "do_trunc: stat size exp %ld obs %ld\n",
                (long)ip->size, (long)ip->stat.st_size);
      return(ERANGE);
  }

/*
 * note the effects of a truncate to the file and globally
 */
  add_trunc(ip, len);

  if (bonus) {
    lt_ino = ip->stat.st_ino;
    lt_len = len;
  }

  return(0);
}
