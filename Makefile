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

db_test: 
	$(CXX) -g -O0 ${HEADER} tests/db_test.cpp -o $@ ${LDLIBS}

db_test2:
	$(CXX) -g -O0 ${HEADER} tests/db_test2.cpp -o $@ ${LDLIBS}

db_test3:
	$(CXX) -g -O0 ${HEADER} tests/db_test3.cpp -o $@ ${LDLIBS}

mp_test: ${MPTEST}
	$(CXX) ${HEADER} $^ -o $@ ${LDLIBS}

aio_test: 
	${CXX} helpers/AIO.h -o aio -std=c++0x -lrt

slice_test:
	$(CXX) -g -O0 ${HEADER} tests/slice_test.cpp -o $@ ${LDLIBS}

clean: 
	rm -f *.o *.idx *.dat test demo* mp_test thread *_test*