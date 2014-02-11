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

batch_test: tests/batch_test.cpp libcustomDB.a
	$(CXX) ${CXXFLAGS} ${HEADER} tests/batch_test.cpp -o $@ ${LDLIBS}

mp_test: 
	$(CXX) ${HEADER} $^ -o $@ ${LDLIBS}

aio_test: 
	${CXX} helpers/AIO.h -o aio -std=c++0x -lrt

db_bench: tests/db_bench.cpp
	$(CXX) ${CXXFLAGS} ${HEADER} $^ -o $@ ${LDLIBS}

db_batch: tests/db_batch.cpp libcustomDB.a
	$(CXX) ${CXXFLAGS} ${HEADER} tests/db_batch.cpp -o $@ ${LDLIBS}

db_batch2: tests/db_batch2.cpp libcustomDB.a
	$(CXX) ${CXXFLAGS} ${HEADER} tests/db_batch2.cpp -o $@ ${LDLIBS}

thread_test: tests/thread_test.cpp
	$(CXX) ${CXXFLAGS} tests/thread_test.cpp -lpthread -o $@ 

db_thread: tests/db_thread.cpp libcustomDB.a
	$(CXX) ${CXXFLAGS} ${HEADER} tests/db_thread.cpp -lpthread -o $@ ${LDLIBS}

rw_test:  tests/rw_test.cpp
	$(CXX) ${CXXFLAGS} tests/rw_test.cpp -lpthread -o $@ 

clean: 
	rm -f *.o *.idx *.dat test demo* mp_test thread *_test* db_bench db_* *log