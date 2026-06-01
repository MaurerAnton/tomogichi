CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O0 -g
INCLUDES := -Isrc
SRC_ENG  := src/core/engine.cpp
SRC_DATA := src/data/storage.cpp
SRC_MAIN := src/main.cpp
OBJ      := build/engine.o build/storage.o build/main.o
TARGET   := tomogichi

.PHONY: all clean run

all: $(TARGET)

build:
	mkdir -p build

build/engine.o: $(SRC_ENG) src/core/types.h src/core/engine.h src/json.hpp | build
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(SRC_ENG) -o $@

build/storage.o: $(SRC_DATA) src/core/types.h src/core/engine.h src/data/storage.h src/json.hpp | build
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(SRC_DATA) -o $@

build/main.o: $(SRC_MAIN) src/core/types.h src/core/engine.h src/data/storage.h src/json.hpp | build
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(SRC_MAIN) -o $@

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $(TARGET)

clean:
	rm -rf build $(TARGET)
	rm -f data/state.json.tmp

run: $(TARGET)
	./$(TARGET)
