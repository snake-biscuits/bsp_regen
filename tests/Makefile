CXX       := g++
CXXFLAGS  := -ggdb --std=c++20 -Wall -O2 -I../src

.PHONY: all run

all: MinMax.exe

run: all
	./MinMax.exe

# TEST EXECUTABLES
MinMax.exe: MinMax.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^
