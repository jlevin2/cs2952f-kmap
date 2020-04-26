
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

    // if ((envwritebuf->empty = sem_open(ENV2SERVEMPTY, O_CREAT | O_RDWR, 0600,
    //                                    0)) == SEM_FAILED) {
    //     perror("sem_open");
    //     exit(1);
    // }

    if (sem_init(&envwritebuf->empty, 1, BUFSIZE)) {
        perror("sem_init");
        exit(1);
    }

    // if ((envwritebuf->full =
    //          sem_open(ENV2SERVFULL, O_CREAT | O_RDWR, 0600, 0)) ==
    //          SEM_FAILED) {
    //     perror("sem_open");
    //     exit(1);
    // }

    if (sem_init(&envwritebuf->full, 1, 0)) {
        perror("sem_init");
        exit(1);
    }

    // if ((servwritebuf->empty = sem_open(SERV2ENVEMPTY, O_CREAT | O_RDWR,
    // 0600,
    //                                     0)) == SEM_FAILED) {
    //     perror("sem_open");
    //     exit(1);
    // }

    if (sem_init(&servwritebuf->empty, 1, BUFSIZE)) {
        perror("sem_init");
        exit(1);
    }

    // if ((servwritebuf->full =
    //          sem_open(SERV2ENVFULL, O_CREAT | O_RDWR, 0600, 0)) ==
    //          SEM_FAILED) {
    //     perror("sem_open");
    //     exit(1);
    // }

    if (sem_init(&servwritebuf->full, 1, 0)) {
        perror("sem_init");
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
    write_log("Starting readv\n");
    size_t numRead;
    // TODO, make this not byte wise (use like memcpy)
    for (numRead = 0; numRead < count; numRead++) {
        if (readbuf->tail == readbuf->head) {
            // Nothing else to read
            return numRead;
        }
        sem_wait(&readbuf->full);
        ((char *)buf)[numRead] = readbuf->data[readbuf->tail];
        readbuf->tail = REALPOS(readbuf->tail + 1);
        sem_wait(&readbuf->empty);
    }

    return numRead;
}

ssize_t circular_write(const void *buf, size_t count) {
    write_log("Before cirular write\nHead: %u\nTail: %u\n", writebuf->head,
              writebuf->tail);

    size_t numWritten = 0;
    for (numWritten = 0; numWritten < count; numWritten++) {
        if (REALPOS(writebuf->head + 1) == writebuf->tail) {
            // FULL,
            return numWritten;
        }

        sem_wait(&writebuf->empty);
        writebuf->data[writebuf->head] = ((char *)buf)[numWritten];
        writebuf->head = REALPOS(writebuf->head + 1);
        sem_post(&writebuf->full);
    }
    write_log("After cirular write\nHead: %u\nTail: %u\n", writebuf->head,
              writebuf->tail);
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
    write_log("Readv with %d iovec \n", iovcnt);
    int i = 0;
    for (i = 0; i < iovcnt; i++) {
        struct iovec curIovec = iov[i];
        ssize_t amtRead = 0;
        write_log("Readv with iovec %d has len %zu \n", i, curIovec.iov_len);
        amtRead = circular_read(curIovec.iov_base, curIovec.iov_len);
        write_log("Readv with iovec %d read amount %zu \n", i, amtRead);
        numRead += amtRead;
        if (amtRead < curIovec.iov_len) {
            write_log("Returned Readv after %d iovec \n", i);
            // POST if there's still data in the buffer:
            break;
        }
    }
    write_log("Returned Readv after %d iovec \n", i);
    // POST if there's still data in the buffer:

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
int buffer_ready() {
    if (!readbuf)
        return 0;

    int val;
    int ret = sem_getvalue(&readbuf->full, &val);
    return ret ? 0 : (val > 0);
}
