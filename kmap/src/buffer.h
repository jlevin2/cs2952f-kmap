#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
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

#define ENV2SERVEMPTY "/ENV2SERVEMTPY"
#define ENV2SERVFULL "/ENV2SERVFULL"

#define SERV2ENVEMPTY "/SERV2ENVEMPTY"
#define SERV2ENVFULL "/SERV2ENVFULL"

#define BUFSIZE 16777216

#ifndef CS2952F_KMAP_BUFFER_H
#define CS2952F_KMAP_BUFFER_H

#define REALPOS(pos) ((pos) % BUFSIZE + BUFSIZE) % BUFSIZE

typedef struct {
    uint32_t head;
    uint32_t tail;
    char data[BUFSIZE];

    // Sync access
    pthread_mutex_t buf_lock;
    pthread_cond_t waiting;
} buffer;

void buffer_setup();

ssize_t buffer_read(void *buf, size_t count);

ssize_t buffer_write(const void *buf, size_t count);

ssize_t buffer_readv(const struct iovec *iov, int iovcnt);

ssize_t buffer_writev(const struct iovec *iov, int iovcnt);

int buffer_ready();

#endif // CS2952F_KMAP_BUFFER_H
