# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g -Wall -shared -fPIC
#gcc -shared -fPIC library.c  -o library.so -ldl

# the build target executable:
TARGET = library
FIFO = fifo
#SOURCE = library

all: run

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) $(TARGET).c $(FIFO).c -o $(TARGET).so -ldl

run: $(TARGET)
	/usr/local/bin/start_static_service.sh

clean:
	$(RM) $(TARGET).so