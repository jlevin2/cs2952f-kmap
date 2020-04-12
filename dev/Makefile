# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g -Wall -shared -fPIC
#gcc -shared -fPIC library.c  -o library.so -ldl

# the build target executable:
TARGET = library
FIFO = buffer
LOGGER = logger
DEBUG = -D DEBUG

all: run

# Compile with ENVOY FLAG SET
envoy: $(TARGET).c $(FIFO).c
	$(CC) $(CFLAGS) -D ENVOY $(TARGET).c $(FIFO).c $(LOGGER).c -o envoy.so -ldl

# Compile with SERVICE FLAG SET
service: $(TARGET).c $(FIFO).c
	$(CC) $(CFLAGS) -D SERVICE $(TARGET).c $(FIFO).c $(LOGGER).c -o service.so -ldl

debug: $(TARGET).c $(FIFO).c
	$(CC) $(CFLAGS) $(DEBUG) -D ENVOY $(TARGET).c $(FIFO).c $(LOGGER).c -o envoy.so -ldl
	$(CC) $(CFLAGS) $(DEBUG) -D SERVICE $(TARGET).c $(FIFO).c $(LOGGER).c -o service.so -ldl

run: clean envoy service
	./start_static_service.sh

clean:
	$(RM) envoy.so service.so library.so shared.so