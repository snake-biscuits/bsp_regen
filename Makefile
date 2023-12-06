CXX       := g++
CXXFLAGS  := -ggdb --std=c++20 -Wall -O2
SRC_DIRS  := src
ALL_DIRS  := $(addprefix build/,$(SRC_DIRS))
CPP_FILES := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.cpp))
HPP_FILES := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.hpp))
O_FILES   := $(foreach file,$(CPP_FILES),build/$(file:.cpp=.o))

DUMMY != mkdir -p build $(ALL_DIRS)

.PHONY: all clean


all: build/convert.exe

clean: rm -rf build/src/


build/%.o: %.cpp $(CPP_FILES)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/convert.exe: $(O_FILES)
	$(CXX) $(CXXFLAGS) -o $@ $^
