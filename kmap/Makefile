# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g -Wall -shared -fPIC
LIBFLAGS = -ldl -lrt

# the build target executable:
TARGET = src/library.c src/buffer.c
DEBUG = -D DEBUG

all: run

# Compile with ENVOY FLAG SET
envoy: $(TARGET)
	$(CC) $(CFLAGS) -D ENVOY $(TARGET) -o envoy.so $(LIBFLAGS)

# Compile with SERVICE FLAG SET
service: $(TARGET)
	$(CC) $(CFLAGS) -D SERVICE $(TARGET) -o service.so $(LIBFLAGS)

tiny: tiny.c
	gcc -D SERVICE -g -o tiny tiny.c -lrt

debug: $(TARGET).c $(FIFO).c
	$(CC) $(CFLAGS) $(DEBUG) -D ENVOY $(TARGET).c $(FIFO).c -o envoy.so -ldl -lrt
	$(CC) $(CFLAGS) $(DEBUG) -D SERVICE $(TARGET).c $(FIFO).c -o service.so -ldl -lrt

run: clean envoy service tiny
	./scripts/run_kmap_service.sh

default: clean tiny
	./scripts/run_default_service.sh

clean:
	$(RM) envoy.so service.so library.so shared.so tiny