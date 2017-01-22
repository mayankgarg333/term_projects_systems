#
# File          : Makefile
# Description   : Build file for CSE473 project 4


# Environment Setup
LIBDIRS=-L. 
INCLUDES=-I. -I/usr/include/
CC=gcc 
CFLAGS=-c $(INCLUDES) -g -m32 -Wall
LINK=gcc -g -m32
LDFLAGS=$(LIBDIRS)
AR=ar rc
RANLIB=ranlib

# Suffix rules
.c.o :
	${CC} ${CFLAGS} $< -o $@

#
# Setup builds

PT-TARGETS=cse473-p3 

# pthreads
LIBS=-l$(UTILLIB) 

#
# Project Protections

p3 : $(PT-TARGETS)

cse473-p3 : cse473-p3.o cse473-util.o cse473-list.o cse473-filesys.o cse473-disk.o 
	$(LINK) $(LDFLAGS) cse473-util.o cse473-list.o cse473-p3.o cse473-filesys.o \
	cse473-disk.o -lm -o $@

clean:
	rm -f *.o *~ $(TARGETS) $(LIBOBJS) lib$(UTILLIB).a

BASENAME=p3
tar: 
	tar cvfz 944916925_P3.tgz -C ..\
	    $(BASENAME)/Makefile \
	    $(BASENAME)/cse473-p3.c \
	    $(BASENAME)/cse473-util.c \
	    $(BASENAME)/cse473-util.h \
	    $(BASENAME)/cse473-list.c \
	    $(BASENAME)/cse473-list.h \
	    $(BASENAME)/cse473-disk.c \
	    $(BASENAME)/cse473-disk.h \
	    $(BASENAME)/cse473-filesys.c \
	    $(BASENAME)/cse473-filesys.h \
	    $(BASENAME)/cmd1 \
	    $(BASENAME)/cmd2 \
	    $(BASENAME)/cmd3 \
	    $(BASENAME)/cmd4 \
	    $(BASENAME)/cmd5 \
	    $(BASENAME)/data1 \
	    $(BASENAME)/data2 \
	    $(BASENAME)/p3-out.cmd1 \
	    $(BASENAME)/p3-out.cmd2 \
	    $(BASENAME)/p3-out.cmd3 \
	    $(BASENAME)/p3-out.cmd4 \
	    $(BASENAME)/p3-out.cmd5
