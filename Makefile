CXX     = g++
AR	    = ar
LIBMISC	= libcustomDB.a
RANLIB  = ranlib
HEADER  = -I./include -I./helpers
CXXFLAGS = -g -O0

SOURCES = cache/*.cpp core/*.cpp helpers/BufferPacket.cpp helpers/HashFunction.cpp

LDLIBS  = -L. -lcustomDB

lib:compile
	${AR} rv ${LIBMISC} *.o
	${RANLIB} ${LIBMISC}

compile:
	${CXX} -g -O0 ${HEADER} -c ${SOURCES} 

thread:
	${CXX}  ${HEADER} helpers/ThreadPool.cpp -o thread -std=c++0x

db_test: tests/db_test.cpp
	$(CXX) ${CXXFLAGS} ${HEADER} $^ -o $@ ${LDLIBS}

db_test2: tests/db_test2.cpp
	$(CXX) ${CXXFLAGS} ${HEADER} $^ -o $@ ${LDLIBS}

db_test3: tests/db_test3.cpp
	$(CXX) ${CXXFLAGS} ${HEADER} $^ -o $@ ${LDLIBS}

slice_test: tests/slice_test.cpp
	$(CXX) ${CXXFLAGS} ${HEADER} $^ -o $@ ${LDLIBS}

mp_test: 
	$(CXX) ${HEADER} $^ -o $@ ${LDLIBS}

aio_test: 
	${CXX} helpers/AIO.h -o aio -std=c++0x -lrt

db_bench: tests/db_bench.cpp
	$(CXX) ${CXXFLAGS} ${HEADER} $^ -o $@ ${LDLIBS}
	
clean: 
	rm -f *.o *.idx *.dat test demo* mp_test thread *_test* db_bench