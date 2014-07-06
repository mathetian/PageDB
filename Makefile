CXX     = g++
AR	    = ar
LIBMISC	= libcustomDB.a
RANLIB  = ranlib
HEADER  = -I./include -I./cache -I./utils -I. -I./core
CXXFLAGS = -g -O0

SOURCES = cache/*.cpp core/*.cpp utils/*.cpp core/PageDB/*.cpp
 
LDLIBS  = -L. -lcustomDB

tests   = test_batch test_buff test_file test_multiplethread test_noncopyable test_reet test_rw test_slice test_timer test_utils
dbtests = db_smalltest db_largetest db_smallbatch db_largebatch db_parallelbatch db_benchmark

lib:compile
	${AR} rv ${LIBMISC} *.o
	${RANLIB} ${LIBMISC}
	rm *.o

compile:
	${CXX} ${CXXFLAGS} ${HEADER} -lpthread -c ${SOURCES} 

dbtests: $(dbtests)
	
db_smalltest: tests/db_smalltest.cpp
	$(CXX) ${CXXFLAGS} ${HEADER} -lpthread -pthread $^ -o $@ ${LDLIBS}

db_largetest: tests/db_largetest.cpp
	$(CXX) ${CXXFLAGS} ${HEADER} -lpthread -pthread $^ -o $@ ${LDLIBS}

db_smallbatch: tests/db_smallbatch.cpp
	$(CXX) ${CXXFLAGS} ${HEADER} -lpthread -pthread $^ -o $@ ${LDLIBS}

db_largebatch: tests/db_largebatch.cpp
	$(CXX) ${CXXFLAGS} ${HEADER} -lpthread -pthread $^ -o $@ ${LDLIBS}

db_parallelbatch: tests/db_parallelbatch.cpp
	$(CXX) ${CXXFLAGS} ${HEADER} -lpthread -pthread $^ -o $@ ${LDLIBS}

db_benchmark: tests/db_benchmark.cpp
	$(CXX) ${CXXFLAGS} ${HEADER} -lpthread -pthread $^ -o $@ ${LDLIBS}

tests: $(tests)

test_slice: tests/test_slice.cpp 
	$(CXX) ${CXXFLAGS} ${HEADER} -lpthread -pthread $^ -o $@ ${LDLIBS}

test_batch: tests/test_batch.cpp
	$(CXX) ${CXXFLAGS} ${HEADER} -lpthread -pthread $^ -o $@ ${LDLIBS}

test_multiplethread: tests/test_multiplethread.cpp
	$(CXX) ${CXXFLAGS} ${HEADER} -lpthread -pthread $^ -o $@ ${LDLIBS}

test_rw:  tests/test_rw.cpp
	$(CXX) ${CXXFLAGS} ${HEADER} -lpthread -pthread $^ -o $@ ${LDLIBS}

test_buff: tests/test_buff.cpp
	$(CXX) ${CXXFLAGS} ${HEADER} -lpthread -pthread $^ -o $@ ${LDLIBS}

test_file: tests/test_file.cpp
	$(CXX) ${CXXFLAGS} ${HEADER} -lpthread -pthread $^ -o $@ ${LDLIBS}

test_noncopyable: tests/test_noncopyable.cpp
	$(CXX) ${CXXFLAGS} ${HEADER} -lpthread -pthread $^ -o $@ ${LDLIBS}

test_reet: tests/test_reet.cpp
	$(CXX) ${CXXFLAGS} ${HEADER} -lpthread -pthread $^ -o $@ ${LDLIBS}

test_utils: tests/test_utils.cpp
	$(CXX) ${CXXFLAGS} ${HEADER} -lpthread -pthread $^ -o $@ ${LDLIBS}

test_timer: tests/test_timer.cpp
	$(CXX) ${CXXFLAGS} ${HEADER} -lpthread -pthread $^ -o $@ ${LDLIBS}

clean: 
	rm -f *.o *.idx *.dat demo* test_* db_* *.txt *.bak