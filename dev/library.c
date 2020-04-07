#define _GNU_SOURCE

#include "fifo.h"

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <sys/uio.h>

typedef int (*socket_t)(int domain, int type, int protocol);

typedef int (*bind_t)(int sockfd, const struct sockaddr *addr,
         socklen_t addrlen);

socket_t real_socket;
bind_t real_bind;

int target_socket = 0;
int cur_accepted_socket = 0;

#define PORT 8080

// Helper to load the real library mapping for use
// Segfaults as of now?
static inline void loadReal(void *pointer, char *func) {
    if (!pointer) {
        pointer = dlsym(RTLD_NEXT, func);
    }
}

int socket(int domain, int type, int protocol) {
//    loadReal(real_socket, "socket");
    if (!real_socket) {
        real_socket = dlsym(RTLD_NEXT, "socket");
    }
    int ret = real_socket(domain, type, protocol);
    fprintf(stderr, "called socket (%d, %d, %d) giving back %d \n", domain, type, protocol, ret);

    return ret;
}

int bind(int socket, const struct sockaddr *address, socklen_t address_len) {
    fprintf(stderr, "called bind socket=%d\n", socket);
    if (!real_bind) {
        real_bind = dlsym(RTLD_NEXT, "bind");
    }
    // CALL BIND SO WE SEE WHICH PORT WAS ASSIGNED
    int bindFD = real_bind(socket, address, address_len);
    struct sockaddr_in my_addr;

    memset(&my_addr, 0, sizeof(my_addr));
    socklen_t len = sizeof(my_addr);
    // this looks up which port the addr is bound to,
    // if bind isn't called yet then it returns null
    getsockname(socket, (struct sockaddr *) &my_addr, &len);
    int myPort = ntohs(my_addr.sin_port);
    fprintf(stderr, "Local port : %d\n", myPort);
    if (myPort == PORT) {
        fprintf(stderr, "Storing socket=%d with port=%d\n", socket, myPort);
        target_socket = socket;
    }

    return bindFD;
}

typedef int (*listen_t)(int socket, int backlog);


listen_t real_listen;

int listen(int socket, int backlog) {
    if (!real_listen) {
        real_listen = dlsym(RTLD_NEXT, "listen");
    }
    fprintf(stderr, "Listen on socket %d\n", socket);

    return real_listen(socket, backlog);
}

//typedef int (*accept_t)(int socket, struct sockaddr *address, socklen_t *address_len);
//
//
//accept_t real_accept;
//
//int accept(int socket, struct sockaddr *address, socklen_t *address_len) {
//    if (!real_accept) {
//        real_accept = dlsym(RTLD_NEXT, "accept");
//    }
//    fprintf(stderr, "Accept on socket %d\n", socket);
//    int returnFD = real_accept(socket, address, address_len);
//    if (socket == target_socket) {
//        fprintf(stderr, "Target socket accepted new connection on %d!\n", returnFD);
//        cur_accepted_socket = returnFD;
//    }
//    return returnFD;
//}

typedef ssize_t (*write_t)(int fd, const void *buf, size_t count);

write_t real_write;

ssize_t write(int fd, const void *buf, size_t count) {
    if (!real_write) {
        real_write = dlsym(RTLD_NEXT, "write");
    }

    // TODO: Once we figure out the fd, just call fifo_write().

//    if (target_socket != 0 && fd == target_socket) {
//        fprintf(stderr, "Write on socket%d, contents=(%s)", fd, buf);
//    }
//    if (fd == target_socket) {
//        fprintf(stderr, "Target socket write!\n");
//    }
//    fprintf(stderr, "Write on socket=%d,len=%zu, contents=%s\n", fd,count, ((char *)buf));
    if (fd == cur_accepted_socket && count > 0) {
        fprintf(stderr, "Write on TARGETSOCKET=%d,len=%zu, contents=%s\n", fd,count, ((char *)buf));
//        for (int i = 0; i < count; i++) {
//            fprintf(stderr, "Char %d = %c\n", i,  ((char *)buf)[i]);
//        }
    }
    return real_write(fd,buf,count);
}

typedef ssize_t (*read_t)(int fd, void *buf, size_t count);

read_t real_read;

ssize_t read(int fd, void *buf, size_t count) {
    if (!real_read) {
        real_read = dlsym(RTLD_NEXT, "read");
    }

    // TODO: Once we figure out the fd, just call fifo_read().
//    if (target_socket != 0 && fd == target_socket) {
//        fprintf(stderr, "Write on socket%d, contents=(%s)", fd, buf);
//    }
    ssize_t res = real_read(fd,buf,count);
//    fprintf(stderr, "READ on socket=%d,len=%zu, contents=%s\n", fd,count, ((char *)buf));
    if (fd == cur_accepted_socket && res > 0) {
        fprintf(stderr, "Read on TARGETSOCKET=%d,len=%zd, contents=%s\n", fd,  res, ((char *)buf));
//        for (int i = 0; i < res; i++) {
//            fprintf(stderr, "Char %d = %c\n", i,  ((char *)buf)[i]);
//        }
    }
//    if (fd == target_socket) {
//        fprintf(stderr, "Target socket read!\n");
//    }
    return res;
}


typedef int (*setsockopt_t)(int socket, int level, int option_name,
                            const void *option_value, socklen_t option_len);
setsockopt_t real_setsockopt;

int setsockopt(int socket, int level, int option_name,
               const void *option_value, socklen_t option_len) {
    if (!real_setsockopt) {
        real_setsockopt = dlsym(RTLD_NEXT, "setsockopt");
    }
//    loadReal(real_setsockopt, "setsockopt");
    int resp = real_setsockopt(socket, level, option_name, option_value, option_len);
    fprintf(stderr, "SETSOCKOPT socket=%d level=%d option_name=%d "
                    "option_value=%s option_len=%zd resp=(%d)\n", socket,
                    level, option_name, option_value, option_len, resp);
    return resp;
}

typedef int (*socketpair_t)(int domain, int type, int protocol, int sv[2]);
socketpair_t real_socketpair;

int socketpair(int domain, int type, int protocol, int sv[2]) {
    if (!real_socketpair) {
        real_socketpair = dlsym(RTLD_NEXT, "socketpair");
    }
//    loadReal(real_socketpair, "socketpair");
    int resp = real_socketpair(domain, type, protocol, sv);
    fprintf(stderr, "SOCKETPAIR domain=%d type=%d protocol=%d "
                    "resp=(%d)\n", domain, type, protocol, resp);
    return resp;
}

typedef int (*connect_t)(int sockfd, const struct sockaddr *addr,
                                socklen_t addrlen);
connect_t real_connect;

int connect(int sockfd, const struct sockaddr *addr,
            socklen_t addrlen) {
    // Helper to load the real library mapping for use
    if (!real_connect) {
        real_connect = dlsym(RTLD_NEXT, "connect");
    }
    int resp = real_connect(sockfd, addr, addrlen);
    struct sockaddr_in *myaddr = (struct sockaddr_in *) addr;
    int myPort = ntohs(myaddr->sin_port);
    char ipAddress[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(myaddr->sin_addr), ipAddress, INET_ADDRSTRLEN);

    fprintf(stderr, "CONNECT sockdf=%d, IP=%s, Port=%d resp=(%d)\n", sockfd, ipAddress, myPort, resp);

    if (resp < 0) {
        perror("Connect returned error code:");
    }
    if (myPort == PORT) {
        fprintf(stderr, "CONNECT setting target FD=%d\n", sockfd);
        target_socket = sockfd;
    }

    return resp;
}

typedef int (*writev_t)(int fildes, const struct iovec *iov, int iovcnt);

writev_t real_writev;

ssize_t writev(int fildes, const struct iovec *iov, int iovcnt) {
    ssize_t resp;

    if (!real_writev) {
        real_writev = dlsym(RTLD_NEXT, "writev");
    }
    if (fildes == target_socket) {
        fprintf(stderr, "WRITEV on target FD=%d\n", fildes);
        resp = fifo_writev(fildes, iov, iovcnt, real_writev);
//        resp = real_writev(fildes, iov, iovcnt);
        fprintf(stderr, "WRITEV returned %d\n", resp);
    } else {
        resp = real_writev(fildes, iov, iovcnt);
    }
    return resp;
}

typedef int (*readv_t)(int fd, const struct iovec *iov, int iovcnt);

readv_t real_readv;

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    ssize_t resp;
    if (!real_readv) {
        real_readv = dlsym(RTLD_NEXT, "readv");
    }
    if (fd == target_socket) {
        fprintf(stderr, "READV on target FD=%d\n", fd);
        resp = fifo_readv(fd, iov, iovcnt, real_readv);
//        resp = real_readv(fd,iov,iovcnt);
        fprintf(stderr, "READV returned %d\n", resp);
    } else {
        resp = real_readv(fd, iov, iovcnt);
    }
    return resp;
}

// Pretty sure this is unnecessary
__attribute__((constructor)) static void setup(void) {
    int ret = fifo_setup(); // set up the fifo
    fprintf(stderr, "%s: called setup()\n", getenv("PATH1"));
    fprintf(stderr, "\n\n%s; %d\n\n", myfifo, ret);
}


