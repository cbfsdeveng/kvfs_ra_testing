#
# kill fs makefile
# $Id$
#
CC=gcc
CFLAGS=-Wall -Werror -g -O
LIBS=-ldl

ALL=kfs

all:	$(ALL)

kfs.o:	kfs.c kfs.h
dir.o:	dir.c kfs.h
random.o:	random.c kfs.h

kfs:	kfs.o dir.o random.o verify.o catch.o
	$(CC) $(CFLAGS) $(LIBS) -o kfs kfs.o dir.o random.o verify.o catch.o

.PHONY:	tags
tags:
	ctags *.[ch]
	etags *.[ch]
	mkid *.[ch]


clean:
	rm -f $(ALL) *.o *~

realclean:
	rm -f $(ALL) *.o *~ ID tags TAGS
