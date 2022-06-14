CXX :=g++
CXXFLAGS :=-std=c++11

all: cache_sim

SOURCES := ./cache.cc ./cache_sim.cc
OBJECTS := $(SOURCES:.cc=.o)

cache_sim: $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o cache_sim $(OBJECTS) 

.cc.o:
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $<

clean:
	rm -f cache_sim *.o
