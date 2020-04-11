#define _GNU_SOURCE

//#include "fifo.h"
#include "buffer.h"
#include "logger.h"

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <sys/uio.h>

/*
 * On Envoy side, look for connect to localhost, designated port
 *
 * On Microservice side, look for accept on designated port
 *
 */

// UNIVERSAL
int inbound_socket = 0;
int connection_socket = 0;

#define PORT 8080

#ifdef SERVICE
typedef int (*bind_t)(int sockfd, const struct sockaddr *addr,
         socklen_t addrlen);
bind_t real_bind;
int bind(int socket, const struct sockaddr *address, socklen_t address_len) {
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
    fprintf(stderr, "Bind called on local port : %d\n", myPort);
    if (myPort == PORT) {
        fprintf(stderr, "Storing socket=%d binded to port=%d\n", socket, myPort);
        inbound_socket = socket;
    }

    return bindFD;
}

// Doesn't matter if listening, just care about accept

typedef int (*listen_t)(int socket, int backlog);
listen_t real_listen;
int listen(int socket, int backlog) {
    if (!real_listen) {
        real_listen = dlsym(RTLD_NEXT, "listen");
    }
    fprintf(stderr, "Listen on socket %d\n", socket);

    return real_listen(socket, backlog);
}

typedef int (*accept_t)(int socket, struct sockaddr *address, socklen_t *address_len);
accept_t real_accept;
int accept(int socket, struct sockaddr *address, socklen_t *address_len) {
    if (!real_accept) {
        real_accept = dlsym(RTLD_NEXT, "accept");
    }
    fprintf(stderr, "Accept on socket %d\n", socket);
    int returnFD = real_accept(socket, address, address_len);
    if (socket == inbound_socket) {
        fprintf(stderr, "Connection socket set to %d!\n", returnFD);
        connection_socket = returnFD;
    }
    return returnFD;
}
#endif

#ifdef ENVOY
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
        fprintf(stderr, "CONNECT on target port, connection FD=%d\n", sockfd);
        connection_socket = sockfd;
    }

    return resp;
}
#endif

// General function wrappers around read/write
typedef ssize_t (*write_t)(int fd, const void *buf, size_t count);
write_t real_write;
ssize_t write(int fd, const void *buf, size_t count) {
    if (!real_write) {
        real_write = dlsym(RTLD_NEXT, "write");
    }

    // TODO: Once we figure out the fd, just call fifo_write().
    ssize_t res = 0;
    if (fd == connection_socket && count > 0) {
        fprintf(stderr, "Buffer write on connection=%d,len=%zd, contents=%s\n", fd,  count, ((char *)buf));
        res = buffer_write(buf, count);
        fprintf(stderr, "Buffer write returned %zu\n", res);
        res = real_write(fd, buf, count);
        fprintf(stderr, "REAL write return %zu \n", res);
    } else {
        res = real_write(fd, buf, count);
    }
    return res;
}

typedef ssize_t (*read_t)(int fd, void *buf, size_t count);

read_t real_read;

ssize_t read(int fd, void *buf, size_t count) {
    if (!real_read) {
        real_read = dlsym(RTLD_NEXT, "read");
    }
    // TODO: Once we figure out the fd, just call fifo_read().
    ssize_t res = 0;
//    ssize_t res = real_read(fd,buf,count);
    if (fd == connection_socket && res > 0) {
        fprintf(stderr, "Buffer Read on connection=%d,len=%zd, contents=%s\n", fd,  count, ((char *)buf));
        res = buffer_read(buf, count);
        fprintf(stderr, "Buffer Read return %zu \n", res);
        res = real_read(fd, buf, count);
        fprintf(stderr, "REAL Read return %zu \n", res);
    } else {
        res = real_read(fd, buf, count);
    }
    return res;
}

typedef int (*writev_t)(int fildes, const struct iovec *iov, int iovcnt);
writev_t real_writev;
ssize_t writev(int fildes, const struct iovec *iov, int iovcnt) {
    ssize_t resp;

    if (!real_writev) {
        real_writev = dlsym(RTLD_NEXT, "writev");
    }
    if (fildes == connection_socket) {
        fprintf(stderr, "BUFFER WRITEV on connection FD=%d\n", fildes);
        resp = buffer_writev(iov, iovcnt);
//        resp = real_writev(fildes, iov, iovcnt);
        fprintf(stderr, "BUFFER WRITEV returned %zu\n", resp);
        resp = real_writev(fildes, iov, iovcnt);
        fprintf(stderr, "REAL WRITEV returned %zu\n", resp);
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
    if (fd == connection_socket) {
        fprintf(stderr, "BUFFER READV on connection FD=%d\n", fd);
        resp = buffer_readv(iov, iovcnt);
//        resp = real_readv(fd,iov,iovcnt);
        fprintf(stderr, "BUFFER READV returned %zu\n", resp);
        resp = real_readv(fd, iov, iovcnt);
        fprintf(stderr, "REAL READV returned %zu\n", resp);
    } else {
        resp = real_readv(fd, iov, iovcnt);
    }
    return resp;
}

// This doesn't work, should initialize if undefined on call (not assuming beforehand)
//__attribute__((constructor)) static void setup(void) {
//    int ret = fifo_setup(); // set up the fifo
//    fprintf(stderr, "%s: called setup()\n", getenv("PATH1"));
//    fprintf(stderr, "\n\n%s; %d\n\n", myfifo, ret);
//}

// DEBUGGING , use make debug to compile library with these functions

#ifdef DEBUG
typedef int (*socket_t)(int domain, int type, int protocol);
socket_t real_socket;
int socket(int domain, int type, int protocol) {
//    loadReal(real_socket, "socket");
    if (!real_socket) {
        real_socket = dlsym(RTLD_NEXT, "socket");
    }
    int ret = real_socket(domain, type, protocol);
    fprintf(stderr, "called socket (%d, %d, %d) giving back %d \n", domain, type, protocol, ret);

    return ret;
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
#endif