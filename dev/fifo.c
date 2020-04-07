//
// Created by Peter Cho on 4/4/20.
//

#include "fifo.h"

int fifo_setup() {
    myfifo = "/tmp/myfifo";

    if (access(myfifo, F_OK) == -1)
        return mkfifo(myfifo, 0666);
    else
        return 2;
}

ssize_t fifo_readv(int fds, const struct iovec *iov, int iovcnt, int (*readv_t)(int, const struct iovec *, int)) {
    int fd = open(myfifo, O_RDWR);

    fprintf(stderr, "\n\n fifo_readv: open attempted \n\n");

    if (fd < 0)
        return fd;

    fprintf(stderr, "\n\n About to Initiate fifo_readv \n\n");

    ssize_t ret = readv_t(fd, iov, iovcnt);

    close(fd);

    return ret;
}

ssize_t fifo_writev(int fds, const struct iovec *iov, int iovcnt, int (*writev_t)(int, const struct iovec *, int)) {
    int fd = open(myfifo, O_RDWR);

    fprintf(stderr, "\n\n fifo_writev: open attempted \n\n");

    if (fd < 0)
        return fd;

    fprintf(stderr, "\n\n About to Initiate fifo_writev \n\n");

    ssize_t ret = writev_t(fd, iov, iovcnt);

    close(fd);

    return ret;
}
