
#include "buffer.h"
#include "logger.h"

// global variables

buffer *readbuf;
buffer *writebuf;
sem_t *readfrom;
sem_t *writeto;

// Ok, so here's the deal,
// WE ASSUME that the envoy library always creates this first--
// Thus, ifdef ENVOY, create the buffer
// ifdef SERVICE, find the buffer
void buffer_setup() {

    write_log("Buffer setup!\n");

    buffer *servwritebuf;
    buffer *envwritebuf;
    sem_t *env2serv;
    sem_t *serv2env;

    if (readbuf || writebuf || readfrom || writeto) {
        assert(readbuf);
        assert(writebuf);
        assert(readfrom);
        assert(writeto);
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

    if ((env2serv = sem_open(ENV2SERV, O_CREAT, 0600, 0)) == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    if ((serv2env = sem_open(SERV2ENV, O_CREAT, 0600, 0)) == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

#ifdef SERVICE
    readbuf = envwritebuf;
    writebuf = servwritebuf;
    readfrom = env2serv;
    writeto = serv2env;
#elif ENVOY
    readbuf = servwritebuf;
    writebuf = envwritebuf;
    readfrom = serv2env;
    writeto = env2serv;
#endif
}

// copies into the cirular buffer and returns the total number of bytes copied
ssize_t circular_read(void *buf, size_t count) {
    write_log("Before cirular read\nHead: %u\nTail: %u\n", readbuf->head,
              readbuf->tail);
    size_t numRead = 0;
    uint32_t end = REALPOS(readbuf->tail + count);
    size_t real_end = -1;
    if (readbuf->tail < readbuf->head) {
        // case 1: [...tail xxxxxxx head...]
        // x represents bytes to be copied.
        real_end = end < readbuf->head ? end : readbuf->head;
        numRead = real_end - readbuf->tail;
        memcpy(buf, readbuf->data + readbuf->tail, numRead);
    } else if (readbuf->tail > readbuf->head) {
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
    } else {
        return 0;
    }

    if (real_end >= 0)
        readbuf->tail = real_end;

    write_log("After cirular read\nHead: %u\nTail: %u\n", readbuf->head,
              readbuf->tail);

    return numRead;
}

ssize_t circular_write(const void *buf, size_t count) {
    write_log("Before cirular write\nHead: %u\nTail: %u\n", writebuf->head,
              writebuf->tail);

    size_t numWritten = 0;
    for (numWritten = 0; numWritten < count; numWritten++) {
        if (REALPOS(writebuf->head + 1) == REALPOS(writebuf->tail)) {
            // FULL,
            return numWritten;
        }
        writebuf->data[writebuf->head] = ((char *)buf)[numWritten];
        writebuf->head = REALPOS(writebuf->head + 1);
    }
    write_log("After cirular write\nHead: %u\nTail: %u\n", writebuf->head,
              writebuf->tail);
    return numWritten;
}

// READ OUT FROM data_buffer into buf
ssize_t buffer_read(void *buf, size_t count) {
    buffer_setup();
    sem_wait(readfrom);
    return circular_read(buf, count);
}

// WRITE TO data_buffer from buf
ssize_t buffer_write(const void *buf, size_t count) {
    buffer_setup();
    ssize_t ret = circular_write(buf, count);
    sem_post(writeto);
    return ret;
}

ssize_t buffer_readv(const struct iovec *iov, int iovcnt) {
    buffer_setup();

    sem_wait(readfrom);
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
    if (numWritten)
        sem_post(writeto);
    return numWritten;
}

// returns 1 if the buffer is ready to read, 0 otherwise
int buffer_ready() {
    if (!readfrom)
        return 0;

    int val;
    int ret = sem_getvalue(readfrom, &val);
    return ret ? 0 : (val > 0);
}
