# Compiler
CC = g++

# Compiler flags
CFLAGS = -Wall -g -std=c++11

# Source files
SOURCES = serverM.cpp serverA.cpp serverR.cpp serverD.cpp client.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Executables
EXECUTABLES = serverM serverA serverR serverD client

# Default target
all: $(EXECUTABLES)

# Rule for building executables
serverM: serverM.o
	$(CC) $(CFLAGS) -o $@ $<

serverA: serverA.o
	$(CC) $(CFLAGS) -o $@ $<

serverR: serverR.o
	$(CC) $(CFLAGS) -o $@ $<

serverD: serverD.o
	$(CC) $(CFLAGS) -o $@ $<

client: client.o
	$(CC) $(CFLAGS) -o $@ $<

# Rule for building object files
%.o: %.cpp
	$(CC) $(CFLAGS) -c $<

# Clean target
clean:
	rm -f $(OBJECTS) $(EXECUTABLES)

# Run targets
run_serverM:
	./serverM

run_serverA:
	./serverA

run_serverR:
	./serverR

run_serverD:
	./serverD

run_client:
	@echo "Usage: ./client <USERNAME> <PASSWORD>"
	@echo "Example: ./client user123 pass456"

# Phony targets
.PHONY: all clean run_serverM run_serverA run_serverR run_serverD run_client