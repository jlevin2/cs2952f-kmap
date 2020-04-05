//
// Created by Peter Cho on 4/4/20.
//

#include "fifo.h"

int fifo_setup() {
    myfifo = "/tmp/myfifo";
    return mkfifo(myfifo, 0600);
}

ssize_t fifo_read(void *buf, size_t count, ssize_t (*read_t)(int, void*, size_t)) {
    int fd = open(myfifo, O_RDONLY);

    if (fd < 0)
        return fd;

    return read_t(fd, (void*) buf, count);
}

ssize_t fifo_write(void *buf, size_t count, ssize_t (*write_t)(int, const void*, size_t)) {
    int fd = open(myfifo, O_WRONLY);

    if (fd < 0)
        return fd;

    return write_t(fd, buf, count);
}
