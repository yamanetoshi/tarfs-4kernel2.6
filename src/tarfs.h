/*
 *  tarfs/tarfs.h
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
 * Version: $Id: tarfs.h,v 1.13 2001/02/25 21:39:03 kaz Exp $
 *
 * Copyright (C) 2001
 * Kazuto Miyoshi (kaz@earth.email.ne.jp)
 * Hirokazu Takahashi (h-takaha@mub.biglobe.ne.jp)
 *
 */

#include "tarinf.h"

/* Debug support */
extern int tarfs_debug;
#define message if (tarfs_debug) printk
#define error printk

/* 'tafs' in little endian */
#define TARFS_SUPER_MAGIC 0x73666174

/* Size, root ino, etc. */
#define TAR_BLOCKSIZE 512
#define TARFS_NAME_LEN 255
#define TARFS_MAX_SIZE	((unsigned long)0x7fffffff)
#define TARFS_ROOT_INO  2

#define TARENT(inode)  ((struct tarent*)((inode)->u.generic_ip))

/* Each file */
struct tarent
{
  /* File attributes */
  umode_t mode;
  uid_t uid;
  gid_t gid;
  loff_t size;
  nlink_t nlink;
  time_t atime;
  time_t ctime;
  time_t mtime;

  /* Offset from start of the archive */
  unsigned long pos;

  /* File name */
  char *name;

  /* Symlink */
  char *linkname;

  /* Inodo nr assigned to the file */
  int ino;

  /* Parent directory, link to children, neighbors */
  struct tarent *parent;
  struct tarent *children;
  struct tarent *neighbors;

  /* Link to hash */
  struct tarent *next_hash;

  /* Link to hardlinked original */
  struct tarent *hardlinked;
};

#define TARENT_HASHSIZE 1024
#define NEXT_HASH(ent)	((ent)->next_hash)

/* Additional information for tarfs superblock */
struct tarfs_sb_info {
  /* 1 if already have archive information */
  int parsed;

  /* Root tarent */
  struct tarent *root_tarent;

  /* Hash table */
  struct tarent **ihash;

  /* Information for statfs */
  unsigned long s_files;
  unsigned long s_blocks;

  /* Max inode assigned */
  unsigned long s_maxino;
};

#define TARSB(sb)  ((struct tarfs_sb_info*)((sb)->u.generic_sbp))

/* Prototypes */
struct tarent *lookup_tarent(struct super_block *sb, unsigned long ino);
struct inode_operations tarfs_dir_inode_operations;
struct inode_operations tarfs_file_inode_operations;
struct file_operations tarfs_file_operations;
struct file_operations tarfs_dir_operations;
int parse_tar(struct super_block *sb);
void delete_all_tarents_and_hash(struct super_block *sb);
ssize_t tarfs_read(struct inode *inode,
		   char * buf, size_t count, loff_t *ppos);
int tarfs_writepage(struct page *page);
int tarfs_readpage(struct file *file, struct page *page);
