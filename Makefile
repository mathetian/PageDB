CXX     = g++
AR	    = ar
LIBMISC	= libcustomDB.a
RANLIB  = ranlib
HEADER  = -I./include -I./helpers
SOURCES = cache/*.cpp core/*.cpp helpers/*.cpp

TESTS   = tests/demo.cpp

LDLIBS  = -L. -lcustomDB

lib:compile
	${AR} rv ${LIBMISC} *.o
	$(MAKE) clean
	${RANLIB} ${LIBMISC}
	rm -f *.o

compile:
	${CXX} ${HEADER} -c ${SOURCES}

test: ${TESTS}
	$(CXX) ${HEADER} $^ -o $@ ${LDLIBS}

clean: 
	rm -f *.o *.idx *.dat test demo*