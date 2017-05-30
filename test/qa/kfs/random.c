/*
 * random test routines for kill fs test
 *
 * $Id$
 */
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <string.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "kfs.h"

/*
 * cumulative roll of the dice and total operations performed
 */
int op_sum[N_OPS];
int op_n[N_OPS];
/*
 * pick a random operation to do
 */
static int
do_a_random(dir_t *top)
{
  int i, err;
  dir_t *dp;
  file_t *fp;
/*
 * 1) if not enough files, then create some
 * 1a) if no dirs, then create some
 * 2) if too many files and we are doing RMDIR_R, waste a top level dir
 * 3) if too many files and we are doing OP_DEL, waste a file
 * 4) roll the dice
 */
  if (nfiles < min_files) {
    i = op_sum[OP_CRE] - 1;
  } else if (ndirs < 5) {
    i = op_sum[OP_MKDIR] - 1;
  } else if (nfiles > max_files && op_dist[OP_RMDIR_R]) {
    i = op_sum[OP_RMDIR_R] - 1;
  } else if (nfiles > max_files && op_dist[OP_DEL]) {
    i = op_sum[OP_DEL] - 1;
  } else {
    i = random() % op_sum[N_OPS - 1];
  }
/*
 * pick an op from the cumulative totals - we make sure that a given
 * op really contributed before we call it.
 */
#define DO_OP(n)    (op_dist[(n)] && i < op_sum[(n)])
/*
 * log which operation we're on
 */
  if (verbose)
    printf("%d: roll %d nf %d nd %d\n", op, i, nfiles, ndirs);
/*
 * directory ops
 */
  if (DO_OP(OP_MKDIR)) {
    if ((err = do_mkdir(pick_dir(top, 1))))
      return(err);
    ++op_n[OP_MKDIR];
  } else if (DO_OP(OP_RMDIR)) {
    if ((err = do_rmdir(pick_dir(top, 0))))
      return(err);
    ++op_n[OP_RMDIR];
  } else if (DO_OP(OP_RMDIR_R)) {
/*
 * don't pick top
 */
    do {
      dp = pick_dir(top, 1);
    } while (dp == top);

    if ((err = do_rmdir_r(dp)))
      return(err);
    ++op_n[OP_RMDIR_R];
  } else if (DO_OP(OP_RENAME)) {
    if ((err = do_rename(pick_file(top), pick_dir(top, 1))))
      return(err);
    ++op_n[OP_RENAME];
/*
 * node ops
 */
  } else if (DO_OP(OP_CRE)) {
    if ((err = do_cre(pick_dir(top, 0))))
      return(err);
    ++op_n[OP_CRE];
  } else if (DO_OP(OP_LINK)) {
    dp = pick_dir(top, 1);
/*
 * pick a real file, not a symlink
 */
    do {
      fp = pick_file(top);
    } while(!fp->info);
    if ((err = do_link(dp, fp)))
      return(err);
    ++op_n[OP_LINK];
  } else if (DO_OP(OP_SYM)) {
    if ((err = do_sym(pick_dir(top, 1), pick_file(top))))
      return(err);
    ++op_n[OP_SYM];
  } else if (DO_OP(OP_DEL)) {
    if ((err = do_del(pick_file(top))))
      return(err);
    ++op_n[OP_DEL];
/*
 * I/O ops
 */
  } else if (DO_OP(OP_SYNC)) {
    u32_t data, off, len;
/*
 * random data
 * off and len have to actually do something, and cannot go over size.
 */
    data = random();
    off = (random() % size) & ~3;
    len = (random() % chunk) & ~3;
   
    if (len < 4)
      len = 4;
    if (off + len > size)
      len = size - off;
/*
 * make sure we pick a real file
 */
    do {
      fp = pick_file(top);
    } while (!fp->info);

    if ((err = do_sync(fp, data, off, len)))
      return(err);
    ++op_n[OP_SYNC];
  } else if (DO_OP(OP_ASYNC)) {
    u32_t data, off, len;
/*
 * random data
 * off and len have to actually do something, and cannot go over size.
 */
    data = random();
    off = (random() % size) & ~3;
    len = (random() % chunk) & ~3;
   
    if (len < 4)
      len = 4;
    if (off + len > size)
      len = size - off;
/*
 * make sure we pick a real file
 */
    do {
      fp = pick_file(top);
    } while (!fp->info);

    if ((err = do_async(fp, data, off, len)))
      return(err);
    ++op_n[OP_ASYNC];
  } else if (DO_OP(OP_TRUNC)) {
/*
 * make sure we pick a real file
 */
    do {
      fp = pick_file(top);
    } while (!fp->info);

    if ((err = do_trunc(fp, random() % size)))
      return(err);
    ++op_n[OP_TRUNC];
  }
  return(0);
}
/*
 * microsecond timer and conversions
 */
double
gethrtime(void)
{
  struct timeval t;

  gettimeofday(&t, 0);
  return(1000000.*t.tv_sec + t.tv_usec);
}

double htos(double hr) {return(hr/1000000.);}
double stoh(double s)  {return(s*1000000.);}

#if 0
static char *name[N_OPS] = {
  "mkdir", "rmdir", "rmdir_r", "rename",
  "create", "link", "symlink", "delete",
  "sync", "async", "truncate"
};
#endif
/*
 * we can use this for all kinds of things...
 */
static void
print_vector(int *vec)
{
  printf("ops: mk %d rm %d rm_r %d : ",
	 vec[OP_MKDIR], vec[OP_RMDIR], vec[OP_RMDIR_R]);
  printf("cr %d del %d ln %d sym %d rn %d : ",
	 vec[OP_CRE], vec[OP_DEL], vec[OP_LINK], vec[OP_SYM], vec[OP_RENAME]);
  printf("wr %d tr %d\n",
	 vec[OP_SYNC], vec[OP_TRUNC]);
}
/*
 * we know that top (at least) has an fd for use, what happens after that
 * is up to us.
 */
int
do_random(dir_t *top)
{
  int	    i, err;
  double    start, last, t0;

  if (verbose) {
    printf("DIST: ");
    print_vector(op_dist);
  }
/*
 * rescale the ratios to a cumulative curve for comparisons
 */
  op_sum[OP_MKDIR] = op_dist[OP_MKDIR];
  for (i = 1; i < N_OPS; i++)
    op_sum[i] = op_sum[i-1] + op_dist[i];
/*
 * run the normal operations
 */
  start = last = gethrtime();

  for (op = 0; op < nops && !do_stop; op++) {
    if ((err = do_a_random(top))) {
      printf("FAIL: op %d err (%s)\n", op, strerror(err));
      return(err);
    } else if (progress && (op % progress) == 0) {
      t0 = gethrtime();
      printf("ops %d dirs %d files %d syms %d time %.3lf %.3lf\n",
	     op, ndirs, nfiles, nsyms, htos(t0 - last), htos(t0 - start));
      print_vector(op_n);
      last = t0;
    }
    if (do_time && (gethrtime() - start >= stoh(do_time))) {
      printf("OPS: %d\n", op+1);
      break;
    }
  }
/*
 * now set it up to fail
 */
/*
 * now run ops to make sure it fails
 */
  if (verbose) {
    printf("TOTAL: ");
    print_vector(op_n);
  }
  return(0);
}
