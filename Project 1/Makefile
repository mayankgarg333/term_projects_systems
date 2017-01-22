#
# File          : Makefile
# Description   : Build file for CSE473 project 2


# Environment Setup
LIBDIRS=-L. 
INCLUDES=-I.
CC=gcc 
CFLAGS=-c $(INCLUDES) -g -Wall
LINK=gcc -g
LDFLAGS=$(LIBDIRS)
AR=ar rc
RANLIB=ranlib

# Suffix rules
.c.o :
	${CC} ${CFLAGS} $< -o $@

#
# Setup builds

PT-TARGETS=cse473-p1
CSE473LIB=
CSE473LIBOBJS=

# proj lib
LIBS=

#
# Project Protections

p3 : $(PT-TARGETS)

cse473-p1 : cse473-p1.o cse473-p1-mfu.o cse473-p1-second.o cse473-p1-enh.o
	$(LINK) $(LDFLAGS) cse473-p1.o cse473-p1-mfu.o cse473-p1-second.o cse473-p1-enh.o -o $@

lib$(CSE473LIB).a : $(CSE473LIBOBJS)
	$(AR) $@ $(CSE473LIBOBJS)
	$(RANLIB) $@

clean:
	rm -f *.o *~ $(TARGETS) $(LIBOBJS) lib$(CSE473LIB).a 

BASENAME=p1
PSUID=944916925
tar: 
	tar cvfz $(PSUID).tgz -C ..\
	    $(BASENAME)/Makefile \
	    $(BASENAME)/cse473-p1.c \
	    $(BASENAME)/cse473-p1.h \
	    $(BASENAME)/cse473-p1-mfu.c \
	    $(BASENAME)/cse473-p1-second.c \
	    $(BASENAME)/cse473-p1-enh.c \
	    $(BASENAME)/input.txt \
	    $(BASENAME)/project1-demo 
