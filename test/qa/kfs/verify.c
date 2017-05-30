/*
 * Verify generated top tree against reality
 *
 * $Id$
 */

#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <dirent.h>

#include <unistd.h>
#include <stdlib.h>

#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "kfs.h"

/*
 * the last write and truncate are recorded on bonus runs so we
 * can match against them.
 */
extern u32_t lw_ino, lw_patt, lw_off, lw_len, lt_ino, lt_len;
/*
 * diff a chunk of somthing against a pattern and count
 */
static int
diff_chunk(void *p, int l, u32_t patt)
{
  int count = 0;
  u32_t *op = (u32_t *)p;

  l /= sizeof(*op);
/*
 * simple diff - do gathering later...
 */
  while(l--) {
    if (*op++ != patt)
      ++count;
  }
  return(count);
}

#if 0
/*
 * find how much is the same in the beginning
 */
static int
nsame(void *p, int l, u32_t patt)
{
  u32_t *op = (u32_t *)p;
  int ll = 0;

  l /= sizeof(*op);

  while(l-- && *op++ == patt)
    ++ll;

  return(ll * sizeof(*op));
}

#endif
/*
 * print out differences in a chunk.
 * bonus_ok says whether we know there is an error (everything is an
 * error then - : ) or whether we are doing a bonus printout (+/- notation
 * for got it or missed it)
 */
static void
pdiff(int bonus_ok, int off, void *p, int l, u32_t patt)
{
  int count = 0;
  u32_t *op = (u32_t *)p, bpatt;

  l /= sizeof(*op);

  while(l) {
/*
 * find anything that's the same
 */
    while(l && *op == patt) {
      --l; ++op; off += sizeof(*op);
    }
/*
 * now see about differences that are the same...
 */
    while (l && *op != patt) {
      count = 0;
      bpatt = *op;
      while(l && *op != patt && *op == bpatt) {
	--l; ++op; ++count;
      }
/*
 * if compare_files thinks there are any errors, then we print
 * them all as errors.  If compare_files thinks they are all bonus,
 * then see if the differences are because we matched or missed the
 * bonus write
 */
      if (count) {
	if (bonus_ok > 0) {
	  if (lw_patt == bpatt) {
	    printf("\t%08x %08x + %x-%x (%x)\n",
	       patt, bpatt, off, off+count*4, count*4);
	  } else {
	    printf("\t%08x %08x - %x-%x (%x)\n",
	       patt, bpatt, off, off+count*4, count*4);
	  }
	} else {
	  printf("\t%08x %08x : %x-%x (%x)\n",
		 patt, bpatt, off, off+count*4, count*4);
	}
	off += count*4;
      }
    }
  }
}
/*
 * compare two files - we take the region list from expected and
 * use it to see about observed's contents.  We only get called if
 * the file attributes pan out.
 */
int
compare_files(file_t *exp, file_t *obs)
{
  int fd, err = 0, chunk, len = obs->info->stat.st_size, off, nb;
  int bonus_ok = 0;
  void *p, *curp;
  region_t *rp = exp->info->r_list;
  char *name = make_name(obs->parent, obs->name);

  if (!len)
    return(0);
/*
 * get the observed file mapped for comparison.
 */
  if ((fd = open(name, O_RDONLY)) < 0 ||
      (p = mmap(0, len, PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED) {
    err = errno;
    printf("compare: can't open/map %s (%s)\n", name, strerror(err));
    close(fd);
    free(name);
    return(err);
  }
  close(fd);
  curp = p;
/*
 * take care of any preceding zero-filled gap, then do the region
 */
  while(rp) {
    if (curp < p + rp->off) {
      chunk = p + rp->off - curp;
      err += diff_chunk(curp, chunk, 0);
      curp += chunk;
    }
    chunk = rp->len;
/*
 * if we are looking for bonus ops that might have partially worked we
 * make sure the differences fall into the last (bonus) write and say
 * it's okay.  If we ever see one that is *not* okay, then we clamp
 * not okay.
 */
    nb = diff_chunk(curp, chunk, rp->data);
    err += nb;
    if (nb) {
      if (bonus && bonus_ok >= 0 &&
	  exp->info->stat.st_ino == lw_ino &&
	  rp->off >= lw_off && rp->off + rp->len <= lw_off + lw_len) {
	bonus_ok = 1;
      } else {
	bonus_ok = -1;
      }
    }
    curp += chunk;
    rp = rp->next;
  }
/*
 * take care of any trailing zeroes
 */
  if (curp < p + len) {
    chunk = p + len - curp;
    err += diff_chunk(curp, chunk, 0);
    curp += chunk;
  }	
/*
 * if there were any differences, let's note them now.
 */
  if (err) {
    printf("ino %d %s mismatch %zd : %s\n",
	   (int) obs->info->stat.st_ino,
	   (bonus_ok > 0) ? "bonus" : "data",
	   err*sizeof(u32_t), name);
    if (bonus_ok > 0)
      printf("\t\tbonus is 0x%x @ %x-%x\n", lw_patt, lw_off, lw_off+lw_len);

    rp = exp->info->r_list;
    curp = p;
/*
 * same as above, except this one prints
 */
    while(rp) {
/*
 * preceding zeroes
 */
      if (curp < p + rp->off) {
	chunk = p + rp->off - curp;
	pdiff(-1, curp - p, curp, chunk, 0);
	curp += chunk;
	off += chunk;
      }
/*
 * then the region
 */
      chunk = rp->len;
      pdiff(bonus_ok, curp - p, curp, chunk, rp->data);
      curp += chunk;
      off += chunk;
      rp = rp->next;
    }
/*
 * take care of any trailing zeroes
 */
    if (curp < p + len) {
      chunk = p + len - curp;
      pdiff(-1, curp -p, curp, chunk, 0);
      curp += chunk;
    }
  }

  munmap(p, len);
  if (err)
    return((bonus_ok > 0) ? 0 : err*sizeof(u32_t));
  else
    return(0);
}
/*
 * verify namespace (and data if we are matching)
 */
int
do_verify(dir_t *exp, dir_t *obs)
{
  int i = 103, err = 0, en, on;
  dir_t *edp, *odp;
  file_t *efp, *ofp;
  info_t *eip, *oip;
/*
 * we presume the name has been checked on the way down
 */
  char *name = strdup(make_name(exp, ""));
/*
 * compare regular files
 */
  en = on = 0;
  efp = exp->f_list;
  ofp = obs->f_list;
/*
 * while there is anything left in either list, we are still comparing
 * if they're equal, great, compare attributes
 * if exp < obs, we have an extra in the expected (failed to create)
 * if exp > obs, we have an extra in the observed (failed to delete)
 */
  while(efp || ofp) {
    if (efp && ofp)
      i = strcmp(efp->name, ofp->name);
    else if (efp)
      i = -1;
    else if (ofp)
      i = 1;
/*
 * now see if we compare attrs (same name), found an extra, or missing
 */
    if (i == 0) {				  /* same name */
/*
 * see what we're dealing with.  Check for regular file of the
 * same (or bonus affected) length and see if we compare contents.
 *
 * Since expected could be a dry run, all we can check is length
 * and link count.
 */      
      eip = efp->info;
      oip = ofp->info;
      if (eip && oip) {
/*
 * see if there are any errors before we compare
 */
	if (!S_ISREG(oip->stat.st_mode)) {
	  i = 1;
	  printf("inode %d not REG mode obs 0x%x : %s%s\n",
		 (int)oip->stat.st_ino,
		 (int)oip->stat.st_mode, name, efp->name);
	}

	if( eip->size != eip->stat.st_size ||
	    eip->size != oip->stat.st_size) {
	  printf("inode %d size mismatch exp 0x%x obs 0x%x/0x%x : %s%s\n",
		 (int)oip->stat.st_ino, eip->size,
		 (int)eip->stat.st_size, (int)oip->stat.st_size,
		 name, efp->name);
	  i = 1;
/*
 * if we're looking for bonus here, it may be that some of the data
 * made it but the inode update to extend didn't.  Check to see that
 * the last write applies to this node and intersects the end of the
 * file.  Make sure the last region is the same as the last write,
 * and clip it to let compare sort out the contents.
 */
	  if (bonus && eip->stat.st_ino == lw_ino &&
	      lw_off < oip->stat.st_size &&
	      lw_off + lw_len > oip->stat.st_size) {

	    region_t *rp = eip->r_list;
/*
 * find the end and match it with the last write and clip
 */
	    while (rp) {
	      if (!rp->next)
		break;
              rp = rp->next;
	    }
	    if (rp && rp->off == lw_off && rp->len == lw_len &&
		rp->data == lw_patt) {
	      rp->len = oip->stat.st_size - rp->off;
	      printf("  missed bonus sizing - allowing compare\n");
	      i = 0;
	    }
	  }
	}

	if(eip->count != oip->stat.st_nlink) {
	  printf("inode %d nlink mismatch exp %d obs %d : %s%s\n",
		 (int)oip->stat.st_ino,
		 (int)eip->count, (int)oip->stat.st_nlink,
		 name, efp->name);
#ifndef bug708_fixed
	  printf("bug#708 being allowed\n");
#else
	  i = 1;
#endif

	}
/*
 * if a previous error or we don't compare, that's an error
 */
	if (i || compare_files(efp, ofp))
	    ++err;

      } else if (!eip && !oip) {
	;					  /* KSS - add readlink */
      } else {
	printf("%s%s: missing info exp %d obs %d\n",
	       name, efp->name, eip != 0, oip != 0);
      }
      ++en; efp = efp->next;
      ++on; ofp = ofp->next;
    } else if (i < 0) {				  /* failed to create */
      ++err;
      printf("%s%s: missing file\n", name, efp->name);
      ++en; efp = efp->next;
    } else if (i > 0) {				  /* failed to delete */
      ++err;
      printf("%s%s: extra file\n", name, ofp->name);
      ++on; ofp = ofp->next;
    }
  }
  if (on != en) {
    ++err;
    printf("%s: file count wrong exp %d obs %d\n", name, en, on);
  }

#if 0
/*
 * let's carry on to the bitter end
 */
  if (err)
    return(err);
#endif
/*
 * whew - now let's take a look at the directories
 */  

  en = on = 0;
  edp = exp->d_list;
  odp = obs->d_list;
/*
 * while there is anything left in either list, we are still comparing
 * if they're equal, great, compare attributes
 * if exp < obs, we have an extra in the expected (failed to create)
 * if exp > obs, we have an extra in the observed (failed to delete)
 */
  while(edp || odp) {
    if (edp && odp)
      i = strcmp(edp->name, odp->name);
    else if (edp)
      i = -1;
    else if (odp)
      i = 1;
/*
 * now see if we compare subdirs (same name), found an extra, or missing
 */
    if (i == 0) {				  /* same name */
      if (do_verify(edp, odp))
	++err;
      ++en; edp = edp->next;
      ++on; odp = odp->next;
    } else if (i < 0) {				  /* failed to create */
      ++err;
      printf("%s: %s missing dir\n", name, edp->name);
      ++en; edp = edp->next;
    } else if (i > 0) {				  /* failed to delete */
      ++err;
      printf("%s: %s extra dir\n", name, odp->name);
      ++on; odp = odp->next;
    }
  }
  if (on != en) {
    ++err;
    printf("%s: dir count wrong exp %d obs %d\n", name, en, on);
  }
  free(name);
  return(err);
}
