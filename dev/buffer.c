
#include "buffer.h"

buffer *data_buffer = 0;
sem_t *semaphore = 0;

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

//    if ((semaphore = sem_open(SEMFILE, O_CREAT, 0600, 0)) == SEM_FAILED) {
//        perror("sem_open");
//        exit(1);
//    }
}

// copies into the cirular buffer and returns the total number of bytes copied
ssize_t circular_read(void *buf, size_t count) {
    size_t numRead = 0;
    uint32_t end = REALPOS(data_buffer->tail + count);
    size_t real_end = -1;
    if (data_buffer->tail < data_buffer->head) {
        // case 1: [...tail xxxxxxx head...]
        // x represents bytes to be copied.
        real_end = end < data_buffer->head ? end : data_buffer->head;
        numRead = real_end - data_buffer->tail;
        memcpy(buf, data_buffer->data + data_buffer->tail, numRead);
    } else if (data_buffer->tail > data_buffer->head) {
        // case 2: [xxxhead ...... tailxxx]
        // x represents bytes to be copied -- two copies may be needed.
        if (end > data_buffer->head) {
            real_end = end;
            numRead = end - real_end;
            memcpy(buf, data_buffer->data + data_buffer->tail, numRead);
        } else {
            real_end = end < data_buffer->head ? end : data_buffer->head;

            size_t p1 = BUFSIZE - data_buffer->tail;
            size_t p2 = real_end;
            numRead = p1 + p2;

            memcpy(buf, data_buffer->data + data_buffer->tail, p1);
            memcpy(buf + p1, data_buffer->data + data_buffer->tail + p1, p2);
        }
    }

    assert(real_end >= 0 && real_end < BUFSIZE && "EMPTY BUFFER -- SEMAPHORE FAILURE!");
    data_buffer->tail = real_end;

    return numRead;
}

ssize_t circular_write(const void *buf, size_t count) {
    size_t numWritten = 0;
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

// READ OUT FROM data_buffer into buf
ssize_t buffer_read(void *buf, size_t count) {
    buffer_setup();
//    sem_wait(semaphore);
    return circular_read(buf, count);
}


// WRITE TO data_buffer from buf
ssize_t buffer_write(const void *buf, size_t count) {
    buffer_setup();
    ssize_t ret = circular_write(buf, count);
//    sem_post(semaphore);
    return ret;
}


ssize_t buffer_readv(const struct iovec *iov, int iovcnt) {
    buffer_setup();
//    sem_wait(semaphore);
    ssize_t numRead = 0;

    for (int i = 0; i < iovcnt; i++) {
        struct iovec curIovec = iov[i];
        ssize_t amtRead = 0;
        amtRead = circular_read(curIovec.iov_base, curIovec.iov_len);
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
        amtWritten = circular_write(curIovec.iov_base, curIovec.iov_len);
        numWritten += amtWritten;
        if (amtWritten < curIovec.iov_len) {
            return numWritten;
        }
    }

//    sem_post(semaphore);
    return numWritten;
}


//// returns 1 if the buffer is ready, 0 otherwise
//int buffer_ready(){
//    int val;
//    int ret = sem_getvalue(semaphore, &val);
//    return ret ? 0 : (val > 0);
//}

