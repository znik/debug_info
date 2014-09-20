TARGET = main
CXX = g++
CXXFLAGS = -Wall -g -O0 -std=c++0x
CXXLIBS = -lelf -ldwarf
DEPS = varinfo_i.hpp varinfo.hpp

all: main

main: main.o varinfo.o scoping.o $(DEPS)
	$(CXX) $(CXXFLAGS) main.o varinfo.o scoping.o -o $(TARGET) $(CXXLIBS)

main.o: main.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -c $<

varinfo.o: varinfo.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -c $<

scoping.o: scoping.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -rf *.o $(TARGET)

