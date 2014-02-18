CXX     = g++
AR	    = ar
LIBMISC	= libcustomDB.a
RANLIB  = ranlib
HEADER  = -I./include -I./helpers -I./cache -I./dbimpl -I./utils -I. -I./core
CXXFLAGS = -g -O0

SOURCES = cache/*.cpp core/*.cpp helpers/*.cpp dbimpl/*/*.cpp utils/*.cpp
 
LDLIBS  = -L. -lcustomDB

lib:compile
	${AR} rv ${LIBMISC} *.o
	${RANLIB} ${LIBMISC}

compile:
	${CXX} -g -O0 ${HEADER} -lpthread -c ${SOURCES} 

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

db_bench:  tests/db_bench.cpp
	$(CXX) ${CXXFLAGS} ${HEADER} $^ -o $@ ${LDLIBS}



db_thread2: tests/db_thread2.cpp libcustomDB.a
	$(CXX) ${CXXFLAGS} ${HEADER} tests/db_thread2.cpp -lpthread -o $@ ${LDLIBS}

slice_test: tests/slice_test.cpp libcustomDB.a
	$(CXX) ${CXXFLAGS} ${HEADER} tests/slice_test.cpp -o $@ ${LDLIBS}

batch_test: tests/batch_test.cpp libcustomDB.a
	$(CXX) ${CXXFLAGS} ${HEADER} tests/batch_test.cpp -o $@ ${LDLIBS}


thread_test: tests/thread_test.cpp
	$(CXX) ${CXXFLAGS} tests/thread_test.cpp -lpthread -o $@ 



rw_test:  tests/rw_test.cpp
	$(CXX) ${CXXFLAGS} tests/rw_test.cpp -lpthread -o $@ 

clean: 
	rm -f *.o *.idx *.dat test demo* mp_test thread *_test* db_bench db_* *log *bak test3 test4