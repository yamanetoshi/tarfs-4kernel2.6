# makefile for tarfs
# Version: $Id: Makefile,v 1.4 2001/02/25 20:25:31 kaz Exp $

HEADERS = tarfs.h tarinf.h
OBJS = file.o inode.o super.o dir.o tarent.o namei.o

CFLAGS = -Wall -O2 -D__KERNEL__ -DMODULE

all:	tarfs.o

$(OBJS):  $(HEADERS)

tarfs.o	: $(OBJS) Makefile
	ld -r -o tarfs.o $(OBJS)

clean:
	rm $(OBJS) tarfs.o
