CXX := g++
CXXFLAGS := -Wall -Wextra -std=c++17

INC_FLAGS := -Iinclude -Istructures/include

SRC := $(wildcard *.cpp) $(wildcard structures/src/*.cpp)

OBJ := $(SRC:%.cpp=build/%.o)

TARGET := main

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $@

build/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INC_FLAGS) -c $< -o $@

clean:
	rm -rf build $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run