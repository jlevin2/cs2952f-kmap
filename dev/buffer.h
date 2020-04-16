#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <assert.h>

#define BFILE "kmapHellYea"
#define BID 69

#define SEMFILE "virtualsem"

// Currently 2^16 byte buffer
#define BUFSIZE 65536


#ifndef CS2952F_KMAP_BUFFER_H
#define CS2952F_KMAP_BUFFER_H

#define REALPOS(pos)    pos % BUFSIZE

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


#endif //CS2952F_KMAP_BUFFER_H

// Buffer setup
// 0 ...... back xxxxxxx front .......END
//

