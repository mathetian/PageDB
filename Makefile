CXX     = g++
AR	    = ar
LIBMISC	= libcustomDB.a
RANLIB  = ranlib
HEADER  = -I./include -I./helpers
SOURCES = cache/*.cpp core/*.cpp helpers/BufferPacket.cpp

TESTS   = tests/dp_test.cpp

MPTEST  = tests/mp_test.cpp

LDLIBS  = -L. -lcustomDB

lib:compile
	${AR} rv ${LIBMISC} *.o
	${RANLIB} ${LIBMISC}

compile:
	${CXX} -g -O0 ${HEADER} -c ${SOURCES} 

thread:
	${CXX}  ${HEADER} helpers/ThreadPool.cpp -o thread -std=c++0x

test: 
	$(CXX) -g -O0 ${HEADER} tests/db_test.cpp -o $@ ${LDLIBS}

mp_test: ${MPTEST}
	$(CXX) ${HEADER} $^ -o $@ ${LDLIBS}

aio_test: 
	${CXX} helpers/AIO.h -o aio -std=c++0x -lrt

clean: 
	rm -f *.o *.idx *.dat test demo* mp_test thread