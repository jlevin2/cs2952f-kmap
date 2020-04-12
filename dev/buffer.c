
#include "buffer.h"

buffer *data_buffer = 0;

// Ok, so here's the deal,
// WE ASSUME that the envoy library always creates this first--
// Thus, ifdef ENVOY, create the buffer
// ifdef SERVICE, find the buffer
void buffer_setup() {
    if (data_buffer)
        return;

    key_t key = ftok(BFILE, BID);
    // Then create the buffer (or attach if exists)
    int shmid = shmget(key, sizeof(buffer), 0666 | IPC_CREAT);
    data_buffer = (buffer *) shmat(shmid, (void *)0, 0);
}

// READ OUT FROM data_buffer into buf
ssize_t buffer_read(void *buf, size_t count) {
    buffer_setup();
    size_t numRead = 0;
    // TODO, make this not byte wise (use like memcpy)
    for (numRead = 0; numRead < count; numRead++) {
        if (REALPOS(data_buffer->tail) == REALPOS(data_buffer->head)) {
            // Nothing else to read
            return numRead;
        }
        ((char *) buf)[numRead] = data_buffer->data[REALPOS(data_buffer->tail)];
        data_buffer->tail = REALPOS(data_buffer->tail + 1);
    }

    return 0;
}

// WRITE TO data_buffer from buf
ssize_t buffer_write(const void *buf, size_t count) {
    buffer_setup();
    size_t numWritten = 0;
    // TODO, make this not byte wise (use like memcpy)
    for (numWritten = 0; numWritten < count; numWritten++) {
        if (REALPOS(data_buffer->head + 1) == REALPOS(data_buffer->tail)) {
            // FULL,
            return numWritten;
        }
        data_buffer->data[REALPOS(data_buffer->head + 1)] = ((char *) buf)[numWritten];
        data_buffer->head = REALPOS(data_buffer->head + 1);
    }

    return numWritten;
}


ssize_t buffer_readv(const struct iovec *iov, int iovcnt) {
    buffer_setup();
    ssize_t numRead = 0;
    for (int i = 0; i < iovcnt; i++) {
        struct iovec curIovec = iov[i];
        ssize_t amtRead = 0;
        amtRead = buffer_read(curIovec.iov_base, curIovec.iov_len);
        numRead += amtRead;
        if (amtRead < curIovec.iov_len) {
            return numRead;
        }
    }
    return numRead;
}


ssize_t buffer_writev(const struct iovec *iov, int iovcnt) {
    buffer_setup();
    ssize_t numWritten = 0;
    for (int i = 0; i < iovcnt; i++) {
        struct iovec curIovec = iov[i];
        ssize_t amtWritten = 0;
        amtWritten = buffer_write(curIovec.iov_base, curIovec.iov_len);
        numWritten += amtWritten;
        if (amtWritten < curIovec.iov_len) {
            return numWritten;
        }
    }
    return numWritten;
}

