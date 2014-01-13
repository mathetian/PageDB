CXX     = g++
AR	    = ar
LIBMISC	= libcustomDB.a
RANLIB  = ranlib
HEADER  = -I./include -I./helpers
SOURCES = cache/*.cpp core/*.cpp helpers/*.cpp

TESTS   = tests/dp_test.cpp

MPTEST  = tests/mp_test.cpp

LDLIBS  = -L. -lcustomDB

lib:compile
	${AR} rv ${LIBMISC} *.o
	${RANLIB} ${LIBMISC}

compile:
	${CXX} ${HEADER} -c ${SOURCES} 

thread:
	${CXX}  ${HEADER} helpers/ThreadPool.cpp -o thread -std=c++0x

test: ${TESTS}
	$(CXX) ${HEADER} $^ -o $@ ${LDLIBS}

mp_test: ${MPTEST}
	$(CXX) ${HEADER} $^ -o $@ ${LDLIBS}

clean: 
	rm -f *.o *.idx *.dat test demo* mp_test thread