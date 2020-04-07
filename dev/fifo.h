//
// Created by Peter Cho on 4/4/20.
//

#ifndef CS2952F_KMAP_FIFO_H
#define CS2952F_KMAP_FIFO_H

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

char* myfifo;

int fifo_setup();
ssize_t fifo_readv(int fds, const struct iovec *iov, int iovcnt, int (*readv_t)(int, const struct iovec *, int));
ssize_t fifo_writev(int fds, const struct iovec *iov, int iovcnt, int (*writev_t)(int, const struct iovec *, int));

#endif //CS2952F_KMAP_FIFO_H
