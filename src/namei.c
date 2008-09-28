/*
 *  tarfs/namei.c
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
 * Version: $Id: namei.c,v 1.8 2001/02/25 21:39:03 kaz Exp $
 *
 * Copyright (C) 2001
 * Kazuto Miyoshi (kaz@earth.email.ne.jp)
 * Hirokazu Takahashi (h-takaha@mub.biglobe.ne.jp)
 *
 */

#include <linux/fs.h>
#include "tarfs.h"

static struct dentry *tarfs_lookup(struct inode * dir, struct dentry *dentry)
{
  struct tarent *ent;

  message("tarfs: tarfs_lookup()\n");

  for(ent=TARENT(dir)->children; ent!=0; ent=ent->neighbors){
    if (!strcmp(ent->name, dentry->d_name.name)){
      d_add(dentry, iget(dir->i_sb, ent->ino));
      goto out;
    }
  }
  d_add(dentry, NULL);

 out:
  return NULL;
}

struct inode_operations tarfs_dir_inode_operations = {
  lookup:	tarfs_lookup,
};

