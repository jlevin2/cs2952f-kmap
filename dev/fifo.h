//
// Created by Peter Cho on 4/4/20.
//

#ifndef CS2952F_KMAP_FIFO_H
#define CS2952F_KMAP_FIFO_H

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

const char* myfifo;

int fifo_setup();
ssize_t fifo_read(void *buf, size_t count, ssize_t (*read_t)(int, void*, size_t));
ssize_t fifo_write(void *buf, size_t count, ssize_t (*write_t)(int, const void*, size_t));

#endif //CS2952F_KMAP_FIFO_H
