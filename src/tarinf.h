/*
 *  tarfs/tarinf.h
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
 * Version: $Id: tarinf.h,v 1.3 2001/02/25 21:39:03 kaz Exp $
 *
 * Origilal file tar-1.13 src/tar.h
 * Modified by
 * Kazuto Miyoshi (kaz@earth.email.ne.jp)
 * Hirokazu Takahashi (h-takaha@mub.biglobe.ne.jp)
 *
 */

/* Information in tar archive */

#define TARFS_REGTYPE  '0'
#define TARFS_AREGTYPE '\0'
#define TARFS_LNKTYPE  '1'
#define TARFS_SYMTYPE  '2'
#define TARFS_CHRTYPE  '3'
#define TARFS_BLKTYPE  '4'
#define TARFS_DIRTYPE  '5'
#define TARFS_FIFOTYPE '6'
#define TARFS_CONTTYPE '7'

#define TARFS_GNU_DUMPDIR  'D'
#define TARFS_GNU_LONGLINK 'K'
#define TARFS_GNU_LONGNAME 'L'
#define TARFS_GNU_MULTIVOL 'M'
#define TARFS_GNU_NAMES    'N'
#define TARFS_GNU_SPARSE   'S'
#define TARFS_GNU_VOLHDR   'V'

/* Mode field */
#define TARFS_SUID     04000
#define TARFS_SGID     02000
#define TARFS_SVTX     01000
#define TARFS_MODEMASK 00777

struct posix_header
{
  char name[100];
  char mode[8];
  char uid[8];
  char gid[8];
  char size[12];
  char mtime[12];
  char chksum[8];
  char typeflag;
  char linkname[100];
  char magic[6];
  char version[2];
  char uname[32];
  char gname[32];
  char devmajor[8];
  char devminor[8];
  char prefix[155];
};

#define SPARSES_IN_EXTRA_HEADER  16
#define SPARSES_IN_OLDGNU_HEADER 4
#define SPARSES_IN_SPARSE_HEADER 21

struct sparse
{
  char offset[12];
  char numbytes[12];
};

/* GNU extra header */
struct extra_header
{
  char atime[12];
  char ctime[12];
  char offset[12];
  char realsize[12];
  char longnames[4];
  char unused_pad1[68];
  struct sparse sp[SPARSES_IN_EXTRA_HEADER];
  char isextended;
};

struct sparse_header
{
  struct sparse sp[SPARSES_IN_SPARSE_HEADER];
  char isextended;
};

struct oldgnu_header
{
  char unused_pad1[345];
  char atime[12];
  char ctime[12];
  char offset[12];
  char longnames[4];
  char unused_pad2;
  struct sparse sp[SPARSES_IN_OLDGNU_HEADER];
  char isextended;
  char realsize[12];
};
