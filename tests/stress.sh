#!/bin/sh

# tarfs.o shuld be insmod-ed in advance.

TESTDIR=/home/kaz/tarfstest
ARCHIVES='acttitle-0.9.0 armadillo-0.3.10 autofs-3.1.3 xmms-0.9.1 apache_1.3.4 asclock-gtk-latest automake-1 xpm-3.4k apache_1.3.9 audiofile-0.1.6 linux-2.4.0apel-9.23 audiofile-0.1.7 linux-2.4.1 autoconf-2.13 xemacs-21.1.9 appl'
    
for a in $ARCHIVES
do 
  echo "Losetup $TESTDIR/$a.tar"
  losetup /dev/loop0 $TESTDIR/$a.tar
  echo "Mount /mnt"
  mount -t tarfs /dev/loop0 /mnt
  echo "Start compare"
  comp.sh /mnt/* $TESTDIR/$a
  echo "Comp done, umount, losetup -d"
  umount /mnt
  losetup -d /dev/loop0
done

