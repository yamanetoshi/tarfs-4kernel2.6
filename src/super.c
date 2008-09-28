/*
 *  tarfs/super.c
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
 * Version: $Id: super.c,v 1.18 2001/02/25 21:39:03 kaz Exp $
 *
 * Copyright (C) 2001
 * Kazuto Miyoshi (kaz@earth.email.ne.jp)
 * Hirokazu Takahashi (h-takaha@mub.biglobe.ne.jp)
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/malloc.h>

#include "tarfs.h"

void tarfs_put_super (struct super_block * sb)
{
  kdev_t dev = sb->s_dev;

  message("tarfs : tarfs_put_super\n");

  /* Drop all tarent hash table */
  delete_all_tarents_and_hash(sb);

  destroy_buffers(dev);

  /* Drop fs dependent sb data */
  if (TARSB(sb)){
    kfree(TARSB(sb));
  }
}

int tarfs_statfs (struct super_block * sb, struct statfs * buf)
{
  message("tarfs : tarfs_statfs\n");

  buf->f_type = TARFS_SUPER_MAGIC;
  buf->f_bsize = sb->s_blocksize;
  buf->f_blocks = TARSB(sb)->s_files;
  buf->f_bfree = 0;
  buf->f_bavail = 0;
  buf->f_files = TARSB(sb)->s_files;
  buf->f_ffree = 0;
  buf->f_namelen = TARFS_NAME_LEN;
  return 0;
}

int tarfs_remount (struct super_block * sb, int * flags, char * data)
{
  message("tarfs : tarfs_remount\n");
  
  if (!(*flags & MS_RDONLY)){
    /* Remount as RDWR */
    return -EINVAL;
  }

  return 0;
}

int check_magic(struct super_block *sb)
{
  struct buffer_head *bh;
  struct posix_header *ph;
  extern int allow_v7_format;
  int ok=0;

  if (allow_v7_format){
    /* Dangerous when corrupted/non-tar file is specified. */
    return 1;
  }

  bh = bread(sb->s_dev, 0, TAR_BLOCKSIZE);
  if (!bh)
    return 0;

  ph = (struct posix_header*)bh->b_data;
  message("magic %s\n", ph->magic);
  ok = !strncmp(ph->magic, "ustar", 5);
  brelse(bh);

  return ok;
}


extern struct super_operations tarfs_sops;
struct super_block * tarfs_read_super (struct super_block * sb, void * data,
				      int silent)
{
  kdev_t dev = sb->s_dev;

  set_blocksize(dev, TAR_BLOCKSIZE);

  message("tarfs: tarfs_read_super()\n");
  message("tarfs: maj %d, min %d\n", MAJOR(dev), MINOR(dev));
  message("tarfs: mount flags: %ld\n", sb->s_flags);
  sb->s_blocksize_bits = 9; /* Log2(TAR_BLOCKSIZE) */
  sb->s_blocksize = TAR_BLOCKSIZE;

  if (!check_magic(sb)){
    printk("tarfs: Not a tar file, or old v7 format\n");
    goto err_out;
  }

  /* Alloc tarfs dependent data */
  TARSB(sb) = kmalloc(sizeof(struct tarfs_sb_info), GFP_KERNEL);
  if (!TARSB(sb)){
    error("tarfs: Can not allocate fs dependent sb data\n");
    goto err_out;
  }

  TARSB(sb)->parsed=0;
  TARSB(sb)->ihash=(struct tarent**)
    kmalloc(sizeof(struct tarent *[TARENT_HASHSIZE]), GFP_KERNEL);
  if (!TARSB(sb)->ihash)
    goto err_out;
  memset(TARSB(sb)->ihash, 0, sizeof(struct tarent *[TARENT_HASHSIZE]));

  TARSB(sb)->s_files=0;
  TARSB(sb)->s_blocks=0;
  TARSB(sb)->s_maxino=1;

  sb->s_op = &tarfs_sops;
  sb->s_root = d_alloc_root(iget(sb, TARFS_ROOT_INO));
  if (!sb->s_root) {
    goto err_out;
  }
  
  /* Read-only fs */
  sb->s_flags = MS_RDONLY;
  sb->s_dirt = 0;

  return sb;
  
 err_out:
  if (TARSB(sb) && TARSB(sb)->ihash){
    kfree(TARSB(sb)->ihash);
  }
  if (TARSB(sb)){
    kfree(TARSB(sb));
  }
  return 0;
}

extern void tarfs_read_inode (struct inode * inode);
extern void tarfs_write_inode (struct inode * inode, int wait);
extern void tarfs_put_inode (struct inode * inode);
extern void tarfs_delete_inode (struct inode * inode);
extern void tarfs_truncate (struct inode * inode);

static struct super_operations tarfs_sops = {
	read_inode:	tarfs_read_inode,
	put_super:	tarfs_put_super,
	statfs:		tarfs_statfs,
	remount_fs:	tarfs_remount,
};

static DECLARE_FSTYPE_DEV(tar_fs_type, "tarfs", tarfs_read_super);

/* Implemented as module */

MODULE_DESCRIPTION("Linux 2.4 Design & Implementation sample module");
MODULE_AUTHOR("Linux Japan magazine");

int tarfs_debug=0;
int allow_v7_format=0;
MODULE_PARM(tarfs_debug, "i");
MODULE_PARM_DESC(debug, "Tarfs debug output flag");
MODULE_PARM(allow_v7_format, "i");
MODULE_PARM_DESC(allow_v7_format, "Allow v7 format (no magic check)");

static int __init init_tar_fs(void)
{
  printk(KERN_INFO "tarfs: init_tar_fs (debug=%d)\n", tarfs_debug);
  return register_filesystem(&tar_fs_type);
}

static void __exit exit_tar_fs(void)
{
  printk(KERN_INFO "tarfs: exit_tar_fs\n");
  unregister_filesystem(&tar_fs_type);
}

EXPORT_NO_SYMBOLS;

module_init(init_tar_fs)
module_exit(exit_tar_fs)
