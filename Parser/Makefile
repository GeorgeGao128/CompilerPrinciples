CXX := g++
CXXFLAGS := -std=c++11 -O2 -Wall -Wextra -Wpedantic
LDFLAGS := 

SRC := Parser.cpp
OBJ := $(SRC:.cpp=.o)
TARGET := parser

.PHONY: all build clean run

all: build

build: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)


