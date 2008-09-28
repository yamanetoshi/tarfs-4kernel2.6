/*
 *  tarfs/dir.c
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
 * Version: $Id: dir.c,v 1.9 2001/02/25 21:39:03 kaz Exp $
 *
 * Copyright (C) 2001
 * Kazuto Miyoshi (kaz@earth.email.ne.jp)
 * Hirokazu Takahashi (h-takaha@mub.biglobe.ne.jp)
 *
 */

#include <linux/fs.h>
#include <linux/malloc.h>

#include "tarfs.h"

static int tarfs_readdir(struct file * filp,
			 void * dirent, filldir_t filldir)
{
  struct tarfs_inode_info *tii;
  struct inode *inode = filp->f_dentry->d_inode;
  int err;
  struct tarent *dir_tarent, *ent;
  int dtype=0;
  int count, stored;

  dir_tarent = TARENT(inode);

  message("tarfs: tarfs_readdir (dir_tarent %p, f_pos %ld)\n",
	  dir_tarent, (long)filp->f_pos);

  stored=0;  
  tii = (struct tarfs_inode_info *)&(inode->u);

  /* entry 0: dot */
  if (filp->f_pos==0){
    err = filldir(dirent, ".", 1,
		  filp->f_pos, inode->i_ino, DT_DIR);
    filp->f_pos++;
    stored=1;
    goto out;
  }

  /* entry 1: dot dot */
  if (filp->f_pos==1){
    err = filldir(dirent, "..", 2,
		  filp->f_pos, dir_tarent->parent->ino, DT_DIR);
    filp->f_pos++;
    stored=1;
    goto out;
  }

  /* . and .. is already read, we start count from 2 */
  ent=dir_tarent->children;
  for(count=2; count<filp->f_pos && ent!=0; count++){
    ent=ent->neighbors;
  }
  
  if (ent==0)
    goto out;

  for(; ent!=0; ent=ent->neighbors){
    if (S_ISDIR(ent->mode))
      dtype=DT_DIR;
    else if (S_ISREG(ent->mode))
      dtype=DT_REG;

    message("tarfs: tarfs_readdir : adding %s\n", ent->name);
    err = filldir(dirent, ent->name, strlen(ent->name),
		filp->f_pos, ent->ino, dtype);
    if (err)
      goto out;

    stored++;
    filp->f_pos++;
  }

 out:
  message("tarfs: tarfs_readdir() done\n");
  return stored;
}

struct file_operations tarfs_dir_operations = {
	read:		generic_read_dir,	/* just put error */
	readdir:	tarfs_readdir,
};
