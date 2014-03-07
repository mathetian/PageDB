CXX     = g++
AR	    = ar
LIBMISC	= libcustomDB.a
RANLIB  = ranlib
HEADER  = -I./include -I./helpers -I./cache -I./dbimpl -I./utils -I. -I./core
CXXFLAGS = -g -O0

SOURCES = cache/*.cpp core/*.cpp helpers/*.cpp dbimpl/*/*.cpp utils/*.cpp
 
LDLIBS  = -L. -lcustomDB -laio

tests   = test_slice test_batch test_thread test_rw
dbtests = db_smalltest db_largetest db_smallbatch db_largebatch db_batch_thread db_parallel_thread

lib:compile
	${AR} rv ${LIBMISC} *.o
	${RANLIB} ${LIBMISC}

compile:
	${CXX} -g -O0 ${HEADER} -lpthread -c ${SOURCES} 

dbtests: $(dbtests)
	
db_smalltest: tests/db_smalltest.cpp libcustomDB.a
	$(CXX) ${CXXFLAGS} ${HEADER} -lpthread -pthread tests/db_smalltest.cpp -o $@ ${LDLIBS}

db_largetest: tests/db_largetest.cpp libcustomDB.a
	$(CXX) ${CXXFLAGS} ${HEADER} -lpthread -pthread tests/db_largetest.cpp -o $@ ${LDLIBS}

db_smallbatch: tests/db_smallbatch.cpp libcustomDB.a
	$(CXX) ${CXXFLAGS} ${HEADER} -lpthread -pthread tests/db_smallbatch.cpp -o $@ ${LDLIBS}

db_largebatch: tests/db_largebatch.cpp libcustomDB.a
	$(CXX) ${CXXFLAGS} ${HEADER} -lpthread -pthread tests/db_largebatch.cpp -o $@ ${LDLIBS}

db_batch_thread: tests/db_batch_thread.cpp libcustomDB.a
	$(CXX) ${CXXFLAGS} ${HEADER} -lpthread -pthread tests/db_batch_thread.cpp -o $@ ${LDLIBS}

db_parallel_thread: tests/db_parallel_thread.cpp libcustomDB.a
	$(CXX) ${CXXFLAGS} ${HEADER} -lpthread -pthread tests/db_parallel_thread.cpp -o $@ ${LDLIBS}

db_leveldb: tests/db_leveldb.cpp libcustomDB.a
	$(CXX) ${CXXFLAGS} ${HEADER} -lpthread -pthread tests/db_leveldb.cpp -o $@ ${LDLIBS}

tests: $(tests)

test_slice: tests/test_slice.cpp libcustomDB.a
	$(CXX) ${CXXFLAGS} ${HEADER} -lpthread -pthread tests/test_slice.cpp -o $@ ${LDLIBS}

test_batch: tests/test_batch.cpp libcustomDB.a
	$(CXX) ${CXXFLAGS} ${HEADER} -lpthread -pthread tests/test_batch.cpp -o $@ ${LDLIBS}

test_thread: tests/test_thread.cpp libcustomDB.a
	$(CXX) ${CXXFLAGS} ${HEADER} -lpthread -pthread tests/test_thread.cpp -o $@ ${LDLIBS}

test_rw:  tests/test_rw.cpp
	$(CXX) ${CXXFLAGS} ${HEADER} -lpthread -pthread tests/test_rw.cpp     -o $@ ${LDLIBS}

test_aio: tests/test_aio.cpp utils/Thread.cpp
	$(CXX) ${CXXFLAGS} ${HEADER} -lpthread -pthread $^     -o $@ ${LDLIBS} -laio

clean: 
	rm -f *.o *.idx *.dat demo* test_* db_* *.txt