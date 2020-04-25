#include <assert.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#define SERVBUF "/SERVBUF"
#define ENVBUF "/ENVBUF"

#define ENV2SERV "/ENV2SERV"
#define SERV2ENV "/SERV2ENV"

// Currently 2^16 byte buffer
#define BUFSIZE 65536
// #define BUFSIZE 1024

#ifndef CS2952F_KMAP_BUFFER_H
#define CS2952F_KMAP_BUFFER_H

#define REALPOS(pos) pos % BUFSIZE

typedef struct {
    uint32_t head;
    uint32_t tail;
    char data[BUFSIZE];
} buffer;

void buffer_setup();

ssize_t buffer_read(void *buf, size_t count);

ssize_t buffer_write(const void *buf, size_t count);

ssize_t buffer_readv(const struct iovec *iov, int iovcnt);

ssize_t buffer_writev(const struct iovec *iov, int iovcnt);

int buffer_ready();

#endif // CS2952F_KMAP_BUFFER_H

// Buffer setup
// 0 ...... back xxxxxxx front .......END
//
