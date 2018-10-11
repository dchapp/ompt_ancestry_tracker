CXX=clang++
CFLAGS= -g -pedantic -std=c++14 -fopenmp -fPIC  #-Wall # -Werror 
INCLUDE= -I/. -I/ompt -I/g/g17/chapp1/repos/LLVM-openmp/build/include -I/usr/tce/packages/boost/boost-1.66.0-mvapich2-2.2-intel-18.0.1/include

LDFLAGS= -L/. 
LD_LIBRARY_FLAGS= -L/. -Wl,-rpath=/usr/tce/packages/intel/intel-16.0.3/tbb/lib/intel64/gcc4.4
LIBS= -lboost_system -ltbb

all: ancestry_tracker 
	

ancestry_tracker: 
	$(CXX) $(CFLAGS) $(INCLUDE) -c ancestry_tracker.cpp -o ancestry_tracker.o
	$(CXX) -shared -o libancestry_tracker.so ancestry_tracker.o $(LDFLAGS) $(LD_LIBRARY_FLAGS) $(LIBS)
	cp libancestry_tracker.so ../lib/libancestry_tracker.so 


clean:
	rm -f *.o
	rm -f *.so 
	rm -f *.a 


