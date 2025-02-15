
#include "buffer.h"
#include "logger.h"

// global variables

buffer *readbuf;
buffer *writebuf;

// Ok, so here's the deal,
// WE ASSUME that the envoy library always creates this first--
// Thus, ifdef ENVOY, create the buffer
// ifdef SERVICE, find the buffer
void buffer_setup() {
    write_log("Starting buffer setup\n");
    buffer *servwritebuf;
    buffer *envwritebuf;

    if (readbuf || writebuf) {
        assert(readbuf);
        assert(writebuf);
        return;
    }

    int fd;

    /* Open physical memory */
    fd = shm_open(SERVBUF, O_CREAT | O_RDWR, 0777);

    if (fd == -1) {
        perror("shm_open");
        exit(1);
    }

    if (ftruncate(fd, sizeof(buffer)) == -1) {
        perror("ftruncate");
        exit(1);
    }

    /* Map BIOS ROM */
    servwritebuf =
        mmap(0, sizeof(buffer), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (servwritebuf == (void *)-1) {
        perror("mmap");
        exit(1);
    }

    /* Open physical memory */
    fd = shm_open(ENVBUF, O_CREAT | O_RDWR, 0777);
    if (fd == -1) {
        perror("shm_open");
        exit(1);
    }

    if (ftruncate(fd, sizeof(buffer)) == -1) {
        perror("ftruncate");
        exit(1);
    }

    /* Map BIOS ROM */
    envwritebuf =
        mmap(0, sizeof(buffer), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (envwritebuf == (void *)-1) {
        perror("mmap");
        exit(1);
    }

#ifdef ENVOY
    memset(servwritebuf, 0, sizeof(buffer));
    memset(envwritebuf, 0, sizeof(buffer));
#endif

    // INIT PTHREAD CONSTRUCTS
    pthread_mutexattr_t attrmutex;
    pthread_mutexattr_init(&attrmutex);
    pthread_mutexattr_setpshared(&attrmutex, PTHREAD_PROCESS_SHARED);

    if (pthread_mutex_init(&servwritebuf->buf_lock, &attrmutex) != 0) {
        printf("\n mutex init has failed\n");
        exit(1);
    }

    if (pthread_mutex_init(&envwritebuf->buf_lock, &attrmutex) != 0) {
        printf("\n mutex init has failed\n");
        exit(1);
    }

    pthread_condattr_t attrcond;
    pthread_condattr_init(&attrcond);
    pthread_condattr_setpshared(&attrcond, PTHREAD_PROCESS_SHARED);

    if (pthread_cond_init(&envwritebuf->waiting, &attrcond) != 0) {
        perror("pthread_cond_init() error");
        exit(1);
    }

    if (pthread_cond_init(&servwritebuf->waiting, &attrcond) != 0) {
        perror("pthread_cond_init() error");
        exit(1);
    }

#ifdef SERVICE
    readbuf = envwritebuf;
    writebuf = servwritebuf;
#elif ENVOY
    readbuf = servwritebuf;
    writebuf = envwritebuf;
#endif

    write_log("Buffer setup complete!\n");
}

// copies into the cirular buffer and returns the total number of bytes copied
ssize_t circular_read(void *buf, size_t count) {
    write_log("Before circular read: Head: %u Tail: %u\n", readbuf->head,
              readbuf->tail);

    size_t numRead = 0;

    // LOCK BUFFER
    pthread_mutex_lock(&readbuf->buf_lock);

    while (readbuf->tail == readbuf->head)
        pthread_cond_wait(&readbuf->waiting, &readbuf->buf_lock);

    // size_t numRead = 0;
    uint32_t end = REALPOS(readbuf->tail + count);
    size_t real_end = -1;
    if (readbuf->tail < readbuf->head) {
        // case 1: [...tail xxxxxxx head...]
        // x represents bytes to be copied.
        real_end = end < readbuf->head ? end : readbuf->head;
        numRead = real_end - readbuf->tail;
        memcpy(buf, readbuf->data + readbuf->tail, numRead);
    } else {
        // case 2: [xxxhead ...... tailxxx]
        // x represents bytes to be copied -- two copies may be needed.
        if (end > readbuf->head) {
            real_end = end;
            numRead = end - readbuf->tail;
            memcpy(buf, readbuf->data + readbuf->tail, numRead);
        } else {
            real_end = end < readbuf->head ? end : readbuf->head;

            size_t p1 = BUFSIZE - readbuf->tail;
            size_t p2 = real_end;
            numRead = p1 + p2;

            memcpy(buf, readbuf->data + readbuf->tail, p1);
            memcpy(buf + p1, readbuf->data, p2);
        }
    }

    if (real_end >= 0)
        readbuf->tail = real_end;

    // SIGNAL ANY WRITERS WAITING
    write_log("After circular read: Head: %u Tail: %u\n", readbuf->head,
              readbuf->tail);

    pthread_mutex_unlock(&readbuf->buf_lock);
    pthread_cond_signal(&readbuf->waiting);

    return numRead;
}

ssize_t circular_write(const void *buf, size_t count) {
    write_log("Before circular write: Head: %u Tail: %u\n", writebuf->head,
              writebuf->tail);

    pthread_mutex_lock(&writebuf->buf_lock);

    size_t numWritten = 0;
    uint32_t last_pos;

    while ((last_pos = REALPOS(writebuf->tail - 1)) == writebuf->head)
        pthread_cond_wait(&writebuf->waiting, &writebuf->buf_lock);

    uint32_t empty_space;

    if (writebuf->head < last_pos) {
        empty_space = last_pos - writebuf->head + 1;
    } else {
        empty_space = BUFSIZE - (writebuf->head - writebuf->tail + 1);
    }

    if (empty_space < count) {
        count = empty_space;
    }

    uint32_t end = REALPOS(writebuf->head + count);
    size_t real_end = -1;

    if (writebuf->head < last_pos) {
        // case 1: [...head xxxxxxx lp...]
        // x represents empty bytes
        real_end = end < last_pos ? end : last_pos;
        numWritten = real_end - writebuf->head + (real_end == last_pos);
        memcpy(writebuf->data + writebuf->head, buf, numWritten);
    } else {
        // case 2: [xxx lp ...... headxxx]
        // x represents empty bytes -- two copies may be needed.
        if (end > writebuf->head) {
            real_end = end;
            numWritten = end - writebuf->head;
            memcpy(writebuf->data + writebuf->head, buf, numWritten);
        } else {
            real_end = end < last_pos ? end : last_pos;

            size_t p1 = BUFSIZE - writebuf->head;
            size_t p2 = real_end + (last_pos == real_end);
            numWritten = p1 + p2;

            memcpy(writebuf->data + writebuf->head, buf, p1);
            memcpy(writebuf->data, buf + p1, p2);
        }
    }

    if (real_end >= 0)
        writebuf->head = real_end;

    // Signal any readers
    write_log("circular write about to signal\n");
    write_log("circular signaling on cond address %p \n",
              (void *)&writebuf->waiting);

    write_log("circular write about to unlock\n");

    write_log("After circular write: Head: %u Tail: %u\n", writebuf->head,
              writebuf->tail);

    pthread_mutex_unlock(&writebuf->buf_lock);
    pthread_cond_signal(&writebuf->waiting);

    return numWritten;
}

// READ OUT FROM data_buffer into buf
ssize_t buffer_read(void *buf, size_t count) {
    return circular_read(buf, count);
}

// WRITE TO data_buffer from buf
ssize_t buffer_write(const void *buf, size_t count) {
    return circular_write(buf, count);
}

ssize_t buffer_readv(const struct iovec *iov, int iovcnt) {
    if (readbuf->head == readbuf->tail)
        return 0;
    ssize_t numRead = 0;
    // write_log("Readv with %d iovec \n", iovcnt);
    int i = 0;
    for (i = 0; i < iovcnt; i++) {
        struct iovec curIovec = iov[i];
        ssize_t amtRead = 0;
        amtRead = circular_read(curIovec.iov_base, curIovec.iov_len);
        numRead += amtRead;
        if (amtRead < curIovec.iov_len) {
            break;
        }
    }

    return numRead;
}

ssize_t buffer_writev(const struct iovec *iov, int iovcnt) {
    ssize_t numWritten = 0;

    for (int i = 0; i < iovcnt; i++) {
        struct iovec curIovec = iov[i];
        ssize_t amtWritten = 0;
        amtWritten = circular_write(curIovec.iov_base, curIovec.iov_len);
        numWritten += amtWritten;
        if (amtWritten < curIovec.iov_len) {
            break;
        }
    }

    return numWritten;
}

// returns 1 if the buffer is ready to read, 0 otherwise
int buffer_ready() { return readbuf->head != readbuf->tail; }
