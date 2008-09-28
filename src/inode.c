/* 
 * tarfs/inode.c
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
 * Version: $Id: inode.c,v 1.13 2001/02/25 21:39:03 kaz Exp $
 *
 * Copyright (C) 2001
 * Kazuto Miyoshi (kaz@earth.email.ne.jp)
 * Hirokazu Takahashi (h-takaha@mub.biglobe.ne.jp)
 *
 */

#include <linux/fs.h>
#include <linux/malloc.h>
#include "tarfs.h"

extern struct address_space_operations tarfs_aops;
extern struct inode_operations tarfs_symlink_inode_operations;
void tarfs_read_inode (struct inode * inode)
{
  struct tarent *ent;

  message("tarfs: tarfs_read_inode(ino=%ld)\n", inode->i_ino);

  ent=lookup_tarent(inode->i_sb, inode->i_ino);
  if (!ent)
    goto err_out;

  inode->i_mode = ent->mode;
  message("tarfs: tarfs_read_inode() -> mode %d\n", inode->i_mode);
  inode->i_uid = ent->uid;
  inode->i_gid = ent->gid;
  inode->i_nlink = ent->nlink;
  inode->i_size = ent->size;
  inode->i_atime = ent->atime;
  inode->i_ctime = ent->ctime;
  inode->i_mtime = ent->mtime;
  TARENT(inode) = ent;
  
  if (S_ISREG(inode->i_mode)) {
    inode->i_fop = &tarfs_file_operations;
    inode->i_mapping->a_ops = &tarfs_aops;
  } else if (S_ISDIR(inode->i_mode)) {
    inode->i_op = &tarfs_dir_inode_operations;
    inode->i_fop = &tarfs_dir_operations;
  } else if (S_ISLNK(inode->i_mode)) {
    inode->i_op = &tarfs_symlink_inode_operations;
  } else {
    /* XXX: dev, fifo, contype etc. */
    error("tarfs: unsupported file type (i_mode=%d)\n", inode->i_mode);
  }

  return;

 err_out:
    make_bad_inode(inode);
    return;
}

/*
 * Getblock for tarfs
 */
static int tarfs_get_block(struct inode *inode, long iblock, struct buffer_head *bh_result, int create)
{
  if (create){
    printk("Can not create file on tarfs\n");
    return -EIO;
  }

  bh_result->b_dev = inode->i_dev;
  bh_result->b_blocknr = TARENT(inode)->pos + iblock;
  bh_result->b_state |= (1UL << BH_Mapped);

  return 0;
}

/*
 * Read and fill a page.
 */
int tarfs_readpage(struct file *file, struct page *page)
{
  return block_read_full_page(page, tarfs_get_block);
}

struct address_space_operations tarfs_aops = {
	readpage: tarfs_readpage,
};

/* symlinks */
static int tarfs_readlink(struct dentry *dentry, char *buffer, int buflen)
{
  char *s = (char *)TARENT(dentry->d_inode)->linkname;
  return vfs_readlink(dentry, buffer, buflen, s);
}

static int tarfs_follow_link(struct dentry *dentry, struct nameidata *nd)
{
  char *s = (char *)TARENT(dentry->d_inode)->linkname;
  return vfs_follow_link(nd, s);
}

struct inode_operations tarfs_symlink_inode_operations = {
	readlink:	tarfs_readlink,
	follow_link:	tarfs_follow_link,
};
