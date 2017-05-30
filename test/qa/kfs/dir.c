/*
 * Directory routines
 *
 * $Id$
 *
 */

#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>

#include <unistd.h>
#include <stdlib.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "kfs.h"

/*
 * add/del child dir in parent dir
 */
void
add_dir(dir_t *parent, dir_t *child)
{
  dir_t *sp;
/*
 * if nothing on the list, then this becomes the list.
 * Otherwise, if < head, it becomes the head
 * Otherwise, found out to whom we should append it.
 */
  if (!parent->d_list) {
    parent->d_list = child;
    child->next = 0;
  } else if (strcmp(child->name, parent->d_list->name) < 0) {
    child->next = parent->d_list;
    parent->d_list = child;
  } else {
    for (sp = parent->d_list ; sp ; sp = sp->next) {
      if (!sp->next || strcmp(child->name, sp->name) < 0) {
	child->next = sp->next;
	sp->next = child;
	break;
      }
    }
    if (!sp) {
      fprintf(stderr, "add_dir: can't add %s\n",
	      make_name(parent, child->name));
      exit(1);
    }
  }
  child->parent = parent;

  ++parent->ndirs;
  ++ndirs;
}
void
del_dir(dir_t *parent, dir_t *child)
{
  dir_t *sp;
/*
 * if the head of the list, just take that, otherwise find out
 * who points to it.
 */
  if (parent->d_list == child) {
    parent->d_list = child->next;
  } else {
    for (sp = parent->d_list; sp ; sp = sp->next) {
      if (sp->next == child) {
	sp->next = child->next;
	break;
      }
    }
    if (!sp) {
      fprintf(stderr, "del_dir: can't find %s\n",
	      make_name(parent, child->name));
      exit(1);
    }
  }
/*
 * now take care of resources (fd, name) and then free up this puppy
 */
  if (child->fd != -1) {
    if(my_close(child->fd)) {
      fprintf(stderr, "del_dir: close on %s failed (%s)\n",
	      make_name(parent, child->name), strerror(errno));
      exit(1);
    }
  }
  if (child->name)
    free(child->name);
  free(child);
/*
 * and let the world know of its demise
 */
  --parent->ndirs;
  --ndirs;
}

/*
 * add/del file in parent dir
 */

void
add_file(dir_t *parent, file_t *child)
{
  file_t *sp;
/*
 * if nothing on the list, then this becomes the list.
 * Otherwise, if < head, it becomes the head
 * Otherwise, found out to whom we should append it.
 */
  if (!parent->f_list) {
    parent->f_list = child;
    child->next = 0;
  } else if (strcmp(child->name, parent->f_list->name) < 0) {
    child->next = parent->f_list;
    parent->f_list = child;
  } else {
    for (sp = parent->f_list ; sp ; sp = sp->next) {
      if (!sp->next || strcmp(child->name, sp->next->name) < 0) {
	child->next = sp->next;
	sp->next = child;
	break;
      }
    }
    if (!sp) {
      fprintf(stderr, "add_file: can't find %s\n",
	      make_name(parent, child->name));
      exit(1);
    }
  }

  child->parent = parent;
  ++parent->nfiles;
/*
 * if it's a file/link, then bump the file count and info count.
 * Otherwise, it's a symlink.
 */
  if (child->info) {
    ++nfiles;
    ++child->info->count;
  } else {
    ++nsyms;
  }
  if (verbose > 1)
    print_tree(parent);
}

void
del_file(dir_t *parent, file_t *child)
{
  file_t *sp;
  region_t *rp, *nrp;
  info_t *ip = child->info;
/*
 * if the head of the list, just take that, otherwise find out
 * who points to it.
 */
  if (parent->f_list == child) {
    parent->f_list = child->next;
  } else {
    for (sp = parent->f_list; sp ; sp = sp->next) {
      if (sp->next == child) {
	sp->next = child->next;
	break;
      }
    }
    if (!sp) {
      fprintf(stderr, "del_file: can't find %s\n",
	      make_name(parent, child->name));
      exit(1);
    }
  }
/*
 * now take care of resources (fd, name, info/regions?)
 */
  if (child->fd != -1) {
    if(my_close(child->fd)) {
      fprintf(stderr, "del_file: close on %s failed (%s)\n",
	      make_name(parent, child->name), strerror(errno));
      exit(1);
    }
  }

  if (child->name)
    free(child->name);
/*
 * info is reference counted with other links
 */
  if (ip) {
    --nfiles;
    if(--ip->count == 0) {
      for (rp = ip->r_list; rp; rp = nrp) {
	nrp = rp->next;
	free(rp);
      }
      free(ip);
    }
  } else {
    --nsyms;
  }

/*
 * free up this puppy and inform the world of the demise
 */
  free(child);

  --parent->nfiles;
}
/*
 * for rename - just move a file from one dir to another.
 * N.B. we always take it out and put it in, even if the dir
 * is the same, as the name may have changed.
 */
void
move_file(dir_t *from, dir_t *to, file_t *child)
{
  file_t *sp;
  
/*
 * remove from old directory.
 * if the head of the list, just take that, otherwise find out
 * who points to it.
 */
  if (from->f_list == child) {
    from->f_list = child->next;
  } else {
    for (sp = from->f_list; sp ; sp = sp->next) {
      if (sp->next == child) {
	sp->next = child->next;
	break;
      }
    }
    if (!sp) {
      fprintf(stderr, "move_del_file: can't find %s\n",
	      make_name(from, child->name));
      exit(1);
    }
  }
  --from->nfiles;
/*
 * add to new parent
 */
  if (!to->f_list) {
    to->f_list = child;
    child->next = 0;
  } else if (strcmp(child->name, to->f_list->name) < 0) {
    child->next = to->f_list;
    to->f_list = child;
  } else {
    for (sp = to->f_list ; sp ; sp = sp->next) {
      if (!sp->next || strcmp(child->name, sp->next->name) < 0) {
	child->next = sp->next;
	sp->next = child;
	break;
      }
    }
    if (!sp) {
      fprintf(stderr, "move_add_file: can't find %s\n",
	      make_name(to, child->name));
      exit(1);
    }
  }
  ++to->nfiles;
  child->parent = to;
  if (verbose > 1)
    print_tree(to);
}
/*
 * move a directory from one place to another
 */
void
move_dir(dir_t *from, dir_t *to, dir_t *child)
{
  dir_t *sp;
  
/*
 * remove from old directory.
 * if the head of the list, just take that, otherwise find out
 * who points to it.
 */
  if (from->d_list == child) {
    from->d_list = child->next;
  } else {
    for (sp = from->d_list; sp ; sp = sp->next) {
      if (sp->next == child) {
	sp->next = child->next;
	break;
      }
    }
    if (!sp) {
      fprintf(stderr, "move_del_dir: can't find %s\n",
	      make_name(from, child->name));
      exit(1);
    }
  }
  --from->ndirs;
/*
 * add to new parent
 */
  if (!to->d_list) {
    to->d_list = child;
    child->next = 0;
  } else if (strcmp(child->name, to->d_list->name) < 0) {
    child->next = to->d_list;
    to->d_list = child;
  } else {
    for (sp = to->d_list ; sp ; sp = sp->next) {
      if (!sp->next || strcmp(child->name, sp->next->name) < 0) {
	child->next = sp->next;
	sp->next = child;
	break;
      }
    }
    if (!sp) {
      fprintf(stderr, "move_add_dir: can't find %s\n",
	      make_name(to, child->name));
      exit(1);
    }
  }
  ++to->ndirs;
  child->parent = to;
}
/*
 * Update the regions with a new write
 * sync indicated with len < 0  KSS - not implemented.
 */
void
add_write(info_t *ip, u32_t data, u32_t off, int len)
{
  region_t *rp, *nrp, *lrp, *new;
  u32_t oend, nend, adj;

  nend = off+len;
/*
 * clip anybody at the beginning that we cover.  This guarantees
 * that we don't have to play with the list head later.
 */
  rp = ip->r_list;
  while (rp && off <= rp->off && (rp->off+rp->len) <= nend) {
    nrp = rp->next;
    free(rp);
    --ip->nregions;
    rp = nrp;
  }
  ip->r_list = rp;
/*
 * go thru the list and truncate/clip/split
 */
  lrp = 0;
  while(rp) {
    nrp = rp->next;
/*
 * if the new end is less than the current beginning, we don't clip anymore
 * if the new start is >= the end of this one, go around.
 */
    if (nend < rp->off)
      break;
    oend = rp->off + rp->len;
    if (off < oend) {
/*
 * if the new offset > old, we either:
 * a) split if nend < oend (and are done)
 * b) truncate the end  (nend >= oend)
 *
 */
      if (off > rp->off) {
	if (nend < oend) {			  /* split */
	  ip->nregions += 2;

	  if (!(new = malloc(sizeof(*new))) ||
	      !(lrp = malloc(sizeof(*lrp)))) {
	    printf("region malloc failed\n");
	    exit(1);
	  }
	  rp->len = off - rp->off;		  /* trunc begin */
	  rp->next = new;

	  new->data = data;			  /* middle (us) */
	  new->off = off;
	  new->len = len;
	  new->next = lrp;
	  
	  lrp->data = rp->data;		  /* end */
	  lrp->off = nend;
	  lrp->len = oend - nend;
	  lrp->next = nrp;
	  return;				  /* we're done! */
	} else {				  /* cover end */
	  rp->len -= (oend - off);
	}
/*
 * if the new offset is <= then the old, we either:
 * a) cover the beginning if nend < oend
 * b) cover the whole thing if oend < nend (clip)
 * c) steal it if off <= and end is the same.
 */
      } else {
	if (nend < oend) {			  /* cover beginning */
	  adj = nend - rp->off;
	  rp->off += adj;
	  rp->len -= adj;
	} else if (oend < nend) {		  /* cover all - clip */
	  if (rp == ip->r_list) {
	    printf("shouldn't clip head\n");
	    exit(1);
	  }
	  while (rp && off <= rp->off && (rp->off+rp->len) <= nend) {
	    nrp = rp->next;
	    free(rp);
	    --ip->nregions;
	    rp = nrp;
	  }
	  lrp->next = nrp;
/*
 * the end is equal and we know that the new offset is <= the old,
 * so just take it.
 */
	} else {
	  rp->off = off;
	  rp->len = len;
	  rp->data = data;
	  return;
	}
      }
    }
    lrp = rp;
    rp = nrp;
  }
/*
 * get new region to add
 */
  if(!(new = malloc(sizeof(*new)))) {
    fprintf(stderr, "Couldn't malloc region\n");
    exit(1);
  } else {
    bzero(new, sizeof(*new));
  }
  new->data = data;
  new->off = off;
  new->len = len;
/*
 * at this point, the list is ordered by offset and there are no
 * overlaps.  If there is no list or we preced the head, we become the head.
 * Otherwise, find out who we precede and insert us.
 */
  rp = ip->r_list;
  if (!rp || off < rp->off) {
    ip->r_list = new;
    new->next = rp;
  } else {
    while(rp) {
      if (!rp->next || off < rp->next->off) {
	new->next = rp->next;
	rp->next = new;
	break;
      }
      rp = rp->next;
    }
  }
/*
 * whew!
 */
  ++ip->nregions;
  return;
}
/*
 * clip out regions based on a truncate
 */
void
add_trunc(info_t *ip, u32_t len)
{
  region_t *rp = ip->r_list, *nrp, *lrp = 0;

  if (!rp)
    return;
/*
 * if the truncate gets rid of the whole list, special case that so
 * we don't have to worry about the list head later on.
 */
  if (len <= rp->off) {
    ip->nregions = 0;
    ip->r_list = 0;
    while(rp) {
      nrp = rp->next;
      free(rp);
      rp = nrp;
    }
    return;
  }
/*
 * go find the first one we affect (whose end is larger than the new end)
 * also save the last one *not* affected so we can nail it.
 */
  while(rp && len >= (rp->off + rp->len)) {
    lrp = rp;
    rp = rp->next;
  }
/*
 * if nobody found, just return
 */
  if (!rp)
    return;
/*
 * if the start is less than the new len, truncate this one and allow
 * it to live by moving on.
 */
  if (rp->off < len) {
    rp->len -= (rp->off + rp->len - len);
    lrp = rp;
    rp = rp->next;
  }
/*
 * and get rid of anybody else
 */
  while(rp) {
    nrp = rp->next;
    --ip->nregions;
    free(rp);
    rp = nrp;
  }

  lrp->next = 0;
}

/*
 * make a full path name looking upwards
 */

static char full_path[2048];

char *
make_name(dir_t *dp, char *name)
{
  if (!dp->parent) {
    sprintf(full_path, "%s/", dp->name);
  } else {
    (void)make_name(dp->parent, 0);
    strcat(full_path, dp->name);
    strcat(full_path, "/");
  }
  if (name)
    strcat(full_path, name);

  return(full_path);
}

/*
 * comparison routine for the qsort() of directory name vector.
 */
static int
ncmp(void *a, void *b)
{
  char **aa = a, **bb = b;

  return(strcmp(*aa, *bb));
}
/*
 * given a dir with an fd in it, fill it in at this level.
 * get_dir_r() recurses downwards using this...
 */

typedef struct dirent dirent_t;

/*
 * match . or ..
 */
static inline int
isdot(const char *d)
{
  if (d[0] != '.') return(0);
  if (d[1] == 0 || (d[1] == '.' && d[2] == 0)) return(1);
  return(0);
}

int
get_dir(dir_t *dirp)
{
  int i, n, nr, nents, fdir;
  char buf[16*1024];
  char **namev, **namevp;
  dirent_t *db, *dp, *de;
  long basep = 0;
  dir_t *ndp;
  file_t *nfp;
  info_t *ip;
  int prefix_len = 0;
  struct stat s;

  fdir = dirp->fd;
  if (prefix)
    prefix_len = strlen(prefix);
/*
 * getdents returns a chunk of a directory. We take the entries there and
 * compress them (dirent overhead -> terminating null) into our chunk.
 * We used to allocate chunks and then coalesce at the end, but this
 * version gives realloc a chance to do a better job by just extending
 * our chunk.  The risk here is that other malloc()s might cause this
 * to be recopied, but looking at malloc() usage overall says we should
 * have a clear field here.  Testing seems to say that it doesn't copy
 * too often, even on large directories.
 * At the end we have a vector of names and a chunk containing the names.
 */
  n = 0;
  namev = (char **)0;

  if ((i = fchdir(fdir)))
    return(i);

  lseek(fdir, 0, 0);
  while ((nr = 
#if 1
	  getdirentries(fdir, buf, sizeof(buf), &basep)
#else
	  getdents(fdir, (dirent_t *)buf, sizeof(buf))
#endif
	 ) > 0) {
/*
 * set up begin/end/pointer for a run thru the entries.
 * KSS - . and .. are not always the first entries, so skip them in the loops
 */
    db = dp = (dirent_t *)buf;
    de = (dirent_t *)(buf + nr);
/*
 * run thru this chunk once once to size it up
 * We do not include 0 inode entries (which BSD leaves in) or those
 * that do not match a specified prefix.
 */
    for(nents = 0;
        dp < de;
        dp = (dirent_t *) ((char *)dp + dp->d_reclen)) {
      if (dp->d_ino &&
	  (!prefix || !strncmp(dp->d_name, prefix, prefix_len)) &&
	  !isdot(dp->d_name)
	  ) {
	++nents;
      }
      if ((dp->d_reclen == 0) || (dp->d_reclen & 3)) {
        printf("BOGUS RECORD LEN %d IN DIR %s buf+%lx\n",
	    dp->d_reclen, dirp->name, (long)((char *)dp - buf));
        exit(1);
      }
    }
/*
 * If there is anything in this chunk, add it to the name vector.
 */
    if (nents) {
      namev = realloc(namev, (n + nents)*sizeof(*namev));

      if (!namev)
        return(ENOMEM);
/*
 * start in on the end of the new vector
 */
      namevp = namev + n;
      n += nents;
/*
 * now run thru the dirents, copying over the name and setting up namev
 * N.B. BSD leave deleted entries in there, so we don't allocate those
 */
      for(dp = db;
          dp < de;
          dp = (dirent_t *) ((char *)dp + dp->d_reclen)) {
	if (dp->d_ino &&
	    (!prefix || !strncmp(dp->d_name, prefix, prefix_len))
	    && !isdot(dp->d_name)
	    )
	  if (!(*namevp++ = strdup(dp->d_name)))
	    return(ENOMEM);
      }
    }
  }
/*
 * if there are any entries, then let's sort them out.
 */
  if (n) {
    dir_t *ldp = 0;
    file_t *lfp = 0;
/*
 * sort the names.
 */
    qsort(namev, n, sizeof(*namev), (void *)ncmp);
/*
 * start filling in entries.
 */
    for (i = 0, namevp = namev; i < n; i++, namevp++) {

      if (lstat(*namevp, &s)) {
        i = errno;
        fprintf(stderr, "stat on %s failed (%s)\n",
                make_name(dirp, *namevp), strerror(i));
        return(i);
      }
/*
 * allocate an entry for this node and fill it in.
 */
      if (S_ISDIR(s.st_mode)) {
	if (!(ndp = malloc(sizeof(*ndp))))
	  return(ENOMEM);

	bzero(ndp, sizeof(*ndp));
	ndp->name = *namevp;
	ndp->fd = -1;
	ndp->parent = dirp;
/*
 * first one starts the list, others append to the last one we added.
 */
	++ndirs;
	if (!dirp->ndirs++) {
	  dirp->d_list = ndp;
	} else {
	  ldp->next = ndp;
	}
	ldp = ndp;
      } else {
	if (!(nfp = malloc(sizeof(*nfp))))
	  return(ENOMEM);

	bzero(nfp, sizeof(*nfp));
	nfp->name = *namevp;
/*
 * regular files (and links) get an info structure.
 * unfortunately, links get duped, but who cares on a compare, we
 * check inode and links separately.
 */
	if (S_ISREG(s.st_mode)) {
	  ++nfiles;
	  if (!(ip = malloc(sizeof(*ip))))
	    return(ENOMEM);

	  bzero(ip, sizeof(*ip));
	  nfp->info = ip;
	  ip->count = 1;
	  ip->stat = s;
	} else {
	  ++nsyms;
	}
	nfp->fd = -1;
	nfp->parent = dirp;
/*
 * first one starts the list, others append.
 */
	if (!dirp->nfiles++) {
	  dirp->f_list = nfp;
	} else {
	  lfp->next = nfp;
	}
	lfp = nfp;
      }
    }
  }
/*
 * free up the names vector
 */
  if (namev)
    free(namev);

  return(0);
}
/*
 * if we see children, go fill them in recursively.
 * N.B. - should do something about open file count...
 */

extern int ndirs, nfiles;

int
get_dir_r(dir_t *dirp)
{
  int err, i;
  dir_t *dp;

#if 0
  printf("doing %s\n", dirp->name);
#endif
  if ((err = get_dir(dirp))) {
    return(err);
  } else {
    for (dp = dirp->d_list ; dp ; dp = dp->next) {
      if ((dp->fd = open(dp->name, O_RDONLY)) < 0) {
	err = errno;
	fprintf(stderr, "get_dir_r: can't open %s (%s)\n",
		dp->name, strerror(err));
	return(err);
      } else if ((err = get_dir_r(dp))) {
	return(err);
      }
      if ((i = fchdir(dirp->fd)))
        return(i);
      close(dp->fd);
      dp->fd = -1;
    }
  }
  return(0);
}
