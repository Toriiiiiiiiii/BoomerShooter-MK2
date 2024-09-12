CC = g++

CCFLAGS = -g -c -Wall -O2
LDFLAGS = -lraylib

SRC = $(wildcard src/*.cpp)
HDR = $(wildcard src/*.hpp)
OBJ = $(SRC:src/%.cpp=bin/obj/%.o)

BIN = bin/exec

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

bin/obj/%.o: src/%.cpp
	$(CC) $(CCFLAGS) -o $@ $<
