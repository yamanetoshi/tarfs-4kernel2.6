/*
 *  tarfs/tarent.c
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Version: $Id: tarent.c,v 1.18 2001/02/25 21:39:03 kaz Exp $
 *
 * Copyright (C) 2001
 * Kazuto Miyoshi (kaz@earth.email.ne.jp)
 * Hirokazu Takahashi (h-takaha@mub.biglobe.ne.jp)
 *
 */

#include <linux/fs.h>
#include <linux/locks.h>
#include <linux/malloc.h>
#include <linux/ctype.h>

#include "tarfs.h"

/* 
 * Octal strtoul for tarfs.
 * tar size, mtime, etc. sometimes NOT null terminated.
 */
unsigned long getval8(char *p, size_t siz)
{
  char buf[512];
  unsigned long val;
  int i;

  if (siz>512 || siz <=0){
    error("tarfs: zero or too long siz for getval8\n");
  }
  
  for(i=0; i<siz; i++){
    if (!isspace(p[i]))	/* '\0' is control character */
      break;
  }

  memcpy(buf, p+i, siz-i);
  buf[siz-i]='\0';

  val=simple_strtoul(buf, NULL, 8);
  message("getval8 [%s] %ld\n", buf, val);

  return val;
}

/*
 * Feed a new ino
 */
unsigned long get_new_ino(struct super_block *sb)
{
  message("tarfs: feeding ino %ld\n", TARSB(sb)->s_maxino);
  return ++(TARSB(sb)->s_maxino);
}

/*
 * Add tarent to hash table
 */
void hash_tarent(struct super_block *sb, struct tarent *t, unsigned long ino)
{
  struct tarent **tentp = TARSB(sb)->ihash+ino%TARENT_HASHSIZE;

  /*
   * Adding prior to the exiting entries.
   * Duplicated entries are hidden by the new one.
   */
  t->next_hash = *tentp;
  *tentp = t;
}

/*
 * Lookup tarent by ino
 */
struct tarent *lookup_tarent(struct super_block *sb, unsigned long ino)
{
  struct tarent *ent;

  if (!TARSB(sb)->parsed){
    message("tarfs: !!! sb->parsed == 0\n");
    if (parse_tar(sb))
      goto err_out;
    TARSB(sb)->parsed=1;
  }

  if (ino < 2){
    error("tarfs: lookup_tarent() no such inode %ld\n", ino);
    return 0;
  }

  for(ent=TARSB(sb)->ihash[ino%TARENT_HASHSIZE];
      ent!=0;
      ent=NEXT_HASH(ent)){
    if (ent->ino==ino){
      return (ent->hardlinked ? ent->hardlinked : ent);
    }
  }
  
 err_out:
  return 0;
}

struct tarent *add_tarent(struct super_block *, unsigned long,
			  struct posix_header *, char *);

/*
 * Fill contents of 'te' according to the posix header 'ph'
 */
void set_tarent(struct super_block *sb,
		struct posix_header *ph, struct tarent *te, int blk)
{
  unsigned long mode;
  time_t mtime;
  struct tarent *link;

  message("set_tarent (blk=%d)\n", blk);

  mode=getval8(ph->mode, sizeof(ph->mode));
  switch(ph->typeflag){
  case TARFS_DIRTYPE:
    /* Directory */
    te->mode = S_IFDIR|(mode & TARFS_MODEMASK);
    /*
     * Do not overwrite size/nlink, because get_new_tarent()
     * and following add_to_parent() correctly count up
     * these value.
     */
    break;

  case TARFS_LNKTYPE:
    /* Hardlinks */
    te->linkname = kmalloc(strlen(ph->linkname)+1, GFP_KERNEL);
    if (!te->linkname){
      error("tarfs: Can not assign linkname\n");
      te->linkname = 0;
    }
    strcpy(te->linkname, ph->linkname);
    message("hardlink %s->%s\n", te->name, te->linkname);

    /* Lookup (preassign) original entry */
    link=add_tarent(sb, 0, 0, te->linkname);
    te->hardlinked = link;
    break;

  case TARFS_SYMTYPE:
    /* Symlinks */
    te->mode = S_IFLNK|(mode & TARFS_MODEMASK);
    te->nlink = 1;
    te->linkname = kmalloc(strlen(ph->linkname)+1, GFP_KERNEL);
    if (!te->linkname){
      error("tarfs: Can not assign linkname\n");
      te->linkname = 0;
    }
    strcpy(te->linkname, ph->linkname);
    message("symlink %s->%s\n", te->name, te->linkname);
    break;

  case TARFS_REGTYPE:
  case TARFS_AREGTYPE:
    /* Regular files */

    te->mode = S_IFREG|(mode & TARFS_MODEMASK);
    /*
     * Overwrite size and nlink, because at the first time,
     * tarent is created as directory
     */
    te->size = getval8(ph->size, sizeof(ph->size));
    te->nlink = 1;
    break;

  default:
    /* Unsupported types, dev, fifo */
    error("Unsupported typeflag %d (%c) blk %d, ignored\n",
	  ph->typeflag, ph->typeflag, blk);
    te->mode = S_IFREG|(mode & TARFS_MODEMASK);
    te->size = getval8(ph->size, sizeof(ph->size));
    te->nlink = 1;
    break;
  }

  mtime=getval8(ph->mtime, sizeof(ph->mtime));
  te->mtime = mtime;
  te->atime = mtime;
  te->ctime = mtime;

  te->uid = getval8(ph->uid, sizeof(ph->uid));
  te->gid = getval8(ph->gid, sizeof(ph->gid));
  te->pos = blk;
}

/*
 * Alloacte a new tarent (default value)
 */
struct tarent *get_new_tarent(char *n, int len, unsigned long ino)
{
  struct tarent *te;

  te = (struct tarent *)kmalloc(sizeof(struct tarent), GFP_KERNEL);
  if (!te)
    goto err_out;
  te->name=kmalloc(len+1, GFP_KERNEL);
  if (!te->name)
    goto err_out;

  strcpy(te->name, n);

  te->linkname=0;

  te->ino=ino;
  te->children=te->neighbors=0;
  te->parent=te;

  /*
   * Default value for the directories which IS NOT contained 
   * in the tar file.
   * For the files contained in the tarfile,
   * set_tarent() overwrites these values.
   */
  te->mtime = 0;
  te->atime = 0;
  te->ctime = 0;

  te->uid = current->fsuid;
  te->gid = current->fsgid;
  te->pos = 0;

  te->nlink = te->size = 2;
  te->mode=S_IFDIR|0777;

  te->hardlinked = 0;

  return te;

 err_out:
  if (te && te->name)
    kfree(te->name);
  if (te)
    kfree(te);

  return 0;
}

void delete_tarent(struct tarent *te)
{
  if (te){
    if (te->name){
      kfree(te->name);
    }
    kfree(te);
  }
}

/*
 * Link c to p as a child tarent
 */
void add_to_parent(struct tarent *p, struct tarent *c)
{
  message("tarfs: add_to_parent %p %p\n", p, c);

  if (!(p && S_ISDIR(p->mode)))
    BUG();

  if (c==0)
    BUG();

  c->parent = p;
  c->neighbors = p->children;
  p->children = c;
  p->size+=1;
  p->nlink+=1;
}

struct tarent *lookup_child_and_chop(struct tarent *p, char *n,
				     char **next, int *len)
{
  char *c;
  struct tarent *pp;
  int i;

  if (p==0)
    BUG();

  message("tarfs: lookup_child_and_chop %p %p\n", p, n);
  if (n){
    message("tarfs: name = %s\n", n);
  }

  /* chop */
  for(c=n, i=0; ; i++, c++){
    if (*c=='/'){
      *c=0;
      *next=c+1;
      break;
    }
    if (*c=='\0'){
      *next=0;
      break;
    }
  }

  message("tarfs: len %d\n", i);
  *len=i;

  for(pp=p->children; pp!=0; pp=pp->neighbors){
    message("pp %p\n", pp);
    if (pp==0){
      message("pp==0 BUG!\n");
      break;
    }else if (pp->name==0){
      message("pp->name==0. BUG!\n");
      break;
    }else{
      message("comparing %s to %s\n", pp->name, n);
    }
    if (!strcmp(pp->name, n)){
      message("tarfs: lookup_child_and_chop found child\n");
      return pp;
    }
  }
  message("tarfs: lookup_child_and_chop found NO child\n");
  return 0;
}

struct tarent *add_until_slash(struct super_block *sb,
			       struct tarent *p, char *n, char **next)
{
  struct tarent *te;
  int len;
  unsigned long ino;

  message("add_until_slash (%s)\n", n);
  te = lookup_child_and_chop(p, n, next, &len);
  if (te){
    return te;
  }

  ino=get_new_ino(sb);

  te = get_new_tarent(n, len, ino);
  if (!te)
    goto err_out;

  hash_tarent(sb, te, ino);
  add_to_parent(p, te);

  return te;

 err_out:
  if (te)
    delete_tarent(te);
  return 0;
}

char *skip_slash(char *n)
{
  for(; *n=='/'; n++)
    ;

  return n;
}

/*
 * Count tar header length from 'blk'
 */
unsigned long count_header_blocks(struct super_block *sb,
				  unsigned long blk, int *fatal)
{
  struct buffer_head *bh;
  int i;
  int blocks=0;
  int typeflag, isextended;

  bh = bread(sb->s_dev, blk, TAR_BLOCKSIZE);
  if (!bh){
    error("tarfs: failed to read headers\n");
    goto err_out;
  }

  blocks++;
  typeflag = ((struct posix_header *)(bh->b_data))->typeflag;
  brelse(bh);

  switch(typeflag){
  case TARFS_REGTYPE:
  case TARFS_AREGTYPE:
  case TARFS_LNKTYPE:
  case TARFS_SYMTYPE:
  case TARFS_CHRTYPE:
  case TARFS_BLKTYPE:
  case TARFS_DIRTYPE:
  case TARFS_FIFOTYPE:
  case TARFS_CONTTYPE:
    /* Posix header only */
    break;
  case TARFS_GNU_DUMPDIR:
  case TARFS_GNU_LONGLINK:
  case TARFS_GNU_LONGNAME:
  case TARFS_GNU_MULTIVOL:
  case TARFS_GNU_NAMES:
  case TARFS_GNU_SPARSE:
  case TARFS_GNU_VOLHDR:
    /* Gnu extra header exists */
    printk("Encountering guu extra header\n");
    bh = bread(sb->s_dev, blk+1, TAR_BLOCKSIZE);
    if (!bh){
      error("tarfs: failed to read extra headers\n");
      goto err_out;
    }
    blocks++;
    isextended = ((struct extra_header *)(bh->b_data))->isextended;
    brelse(bh);
    if (isextended)
      goto out;

    /* Gnu sparse header exists */
    printk("Encountering guu sparce header\n");
    for(i=0;;i++, blocks++){
      bh = bread(sb->s_dev, blk+2+i, TAR_BLOCKSIZE);
      if (!bh){
	error("tarfs: failed to read extra headers\n");
	goto err_out;
      }
      isextended = ((struct sparse_header *)(bh->b_data))->isextended;
      brelse(bh);
      if (isextended)
	break;
    }
    break;
  default:
    error("tarfs: invalid or unsupported typeflag. Headers can not be skipped correctly.\n");
    goto err_out;
  }

 out:
  message("non fatal %d\n", blocks);
  *fatal=0;
  return blocks;

 err_out:
  message("fatal\n");
  *fatal=1;
  return 0;
}

/*
 * Add tarent
 * ph != 0, name==0 : normal use.
 * ph == 0, name!=0 : just pre-assign a entry (for hardlink)
 */
struct tarent *add_tarent(struct super_block *sb, unsigned long blk,
			  struct posix_header *ph, char *name)
{
  char *n=0;
  struct tarent *te=0;
  struct tarent *parent;

  /* lookup_tarent can not be used here */
  parent = TARSB(sb)->root_tarent;

  n = (name ? name : ph->name);

  message("Adding tarentry [%s]\n", ph->name);

  for(;;){
    if (n==0){
      message("tarfs: no more components(1).\n");
      goto out;
    }

    n = skip_slash(n);
    if (*n=='\0'){
      message("tarfs: no more components(2).\n");
      goto out;
    }
    if ( *n=='.' && (*(n+1)=='/' || *(n+1)=='\0') ){
      message("tarfs: skipping dot\n");
      n++;
      continue;
    }
    if ( *n=='.' && *(n+1)=='.' && (*(n+2)=='/' || *(n+2)=='\0') ){
      n+=2;
      if (parent==TARSB(sb)->root_tarent){
	message("tarfs: Oops going higher than root, ignored.\n");

	/* Drop this entry */
	goto err_out;
      }
      parent=parent->parent;

      continue;
    }
    
    te=add_until_slash(sb, parent, n, &n);
    parent = te;
  }

 out:
  if (ph)
    set_tarent(sb, ph, parent, blk);
 
 err_out:
  return parent;
}

/*
 * Deleet all tarents and hash.
 */
void delete_all_tarents_and_hash(struct super_block *sb)
{
  struct tarent *te, **tp;
  int i;

  if ( (tp=TARSB(sb)->ihash)==0 )
    return;
  
  for(i=0; i<TARENT_HASHSIZE; i++){
    for(te=*(tp+i); te!=0; te=NEXT_HASH(te))
      delete_tarent(te);
  }
  kfree(TARSB(sb)->ihash);
}

/*
 * Parse tar archive and gather necessary information
 */
int parse_tar(struct super_block *sb)
{
  struct buffer_head *bh;
  struct tarent **tent=0, *root;
  int tarblk, fatal;
  struct posix_header *ph;
  kdev_t dev;
  unsigned long size, headers;

  dev = sb->s_dev;
  printk("tarfs: parse start (%d,%d)\n", MAJOR(dev), MINOR(dev));

  /* create dummy root */
  root=get_new_tarent("root", 4, 2);
  if (!root)
    goto err_out;
  hash_tarent(sb, root, get_new_ino(sb) /* 2 */);
  root->mode=S_IFDIR|0777;
  TARSB(sb)->root_tarent=root;

  tarblk=0;
  for(;;){
    bh = bread(dev, tarblk, TAR_BLOCKSIZE);
    if (!bh){
      /* XXX: disk error or end-of-device */
      break;
    }

    ph = (struct posix_header*)bh->b_data;
    if (ph->name[0] == '\0'){
      brelse(bh);
      break;
    }

    /* Skip headers */
    headers=count_header_blocks(sb, tarblk, &fatal);
    if (fatal){
      error("Encountering unsupported typeflag blk %d, abort parsing\n",
	    tarblk);
      break;
    }

    tarblk+=headers;
    add_tarent(sb, tarblk, ph, 0);

    /* Skip contents of the file */
    size=getval8(ph->size, sizeof(ph->size));
    tarblk += (size+TAR_BLOCKSIZE-1)/TAR_BLOCKSIZE;

    brelse(bh);
  }

  printk("tarfs: parse done (%ld inodes created)\n",
	 TARSB(sb)->s_maxino-2);

  /* XXX: Poor statistics info  */
  TARSB(sb)->s_files = TARSB(sb)->s_maxino-2;
  TARSB(sb)->s_blocks = tarblk;

  return 0;

 err_out:
  if (tent)
    *tent = 0;
  if (root)
    delete_tarent(root);
  return -1;
}
