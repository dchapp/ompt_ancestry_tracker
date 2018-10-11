CXX=clang++ 
CFLAGS= -g -fopenmp -pedantic # -Wall -Werror -std=c++11
INCLUDE= -I/. -I/g/g17/chapp1/repos/LLVM-openmp/build/include

LDFLAGS= #-L/. -L/g/g17/chapp1/repos/ompt_tools/ancestry_tracker/lib -Wl,-rpath=/g/g17/chapp1/repos/ompt_tools/ancestry_tracker/lib/libancestry_tracker.so
LD_LIBRARY_FLAGS= -L/. -L/g/g17/chapp1/repos/ompt_tools/ancestry_tracker/lib -Wl,-rpath=/g/g17/chapp1/repos/ompt_tools/ancestry_tracker/lib/libancestry_tracker.so 
LIBS= -lancestry_tracker

all: diamond_dependency

.PHONY: diamond_dependency
diamond_dependency: diamond_dependency.o 
	$(CXX) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LD_LIBRARY_FLAGS) $(LIBS)

%.o: %.cpp
	$(CXX) -c $(CPPFLAGS) $(CFLAGS) $(INCLUDE) $< -o $@


clean:
	rm -f *.o
	rm -f minimal parallel_region_example parallel_region_example_with_data_race fib tree_traversal process_linked_list hello


