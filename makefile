# Makefile for addhead utility

.PHONY: clean

CC = gcc
BIND = gcc
RM = rm

#   CFLAGS    flags for C compile
#   LFLAGS1   flags after output file spec, before obj file list
#   LFLAGS2   flags after obj file list (libraries, etc)

CFLAGS = -O2 -O3 -DUNIX
LFLAGS1 =
LFLAGS2 = -s 

ADDHEAD_O=	addhead.o opth.o

addhead: $(ADDHEAD_O)
	$(BIND)  $(ADDHEAD_O) -o addhead $(LFLAGS1) $(LFLAGS2) $(LIBS)

clean:
	rm -rf *.o
	rm -rf *.exe
	rm -rf addhead
	