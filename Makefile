WKDIR=..

CXX = g++

COMPILE.c=$(CC) $(CFLAGS) $(CPPFLAGS) -c
LINK.c=$(CC) $(CFLAGS) $(CPPFLAGS) $(LDDIR) $(LDFLAGS)

LDDIR=-L../include
CFLAGS=-DLINUX -ansi -I$(WKDIR)/include -Wall -D_GNU_SOURCE $(EXTRA)

AR	= ar
LIBMISC	= libcustomDB.a

OBJS   = CustomDB.o

RANLIB     = ranlib

PROGS = CHash EHash CustomDB Log LMC FFLMC LRULMC BaseCache

CXXFLAGS = -I./include -I./helpers

SOURCES=cache/*.cpp core/*.cpp helpers/*.cpp

all:
	${CXX} ${CXXFLAGS} -c ${SOURCES}

lib:${OBJS}
	${AR} rv ${LIBMISC} *.o
	$(MAKE) clean
	${RANLIB} ${LIBMISC}
	rm -f *.o

clean:
	rm -f ${PROGS} ${TEMPFILES} *.o *.idx *.dat all