CXX = g++
CXXFLAGS = -Wall -O3 -std=c++0x
CXXLIBS = -lelf -ldwarf
#DEPS = varinfo_i.hpp varinfo.hpp

all: libdebug_info.a

libdebug_info.a: varinfo.o scoping.o
	ar rcs $@ varinfo.o scoping.o
	ranlib $@

varinfo.o: varinfo.cpp
	$(CXX) $(CXXFLAGS) -c $<

scoping.o: scoping.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -rf *.o libdebug_info.a

