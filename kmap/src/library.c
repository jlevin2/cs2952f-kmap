#define _GNU_SOURCE

#include "buffer.h"
#include "logger.h"

#include <arpa/inet.h>
#include <dlfcn.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>

/*
 * On Envoy side, look for connect to localhost, designated port
 *
 * On Microservice side, look for accept on designated port
 *
 */

#define SERVICE_PORT 8080

// UNIVERSAL
int connection_socket = 0;

int segfault() {
    int *ptr = NULL;
    return *ptr;
}

#ifdef SERVICE
#define SERVICE_PORT 8080
#define SERVICE_ADDRESS "127.0.0.1"

typedef int (*accept_t)(int socket, struct sockaddr *restrict address,
                        socklen_t *restrict address_len);
accept_t real_accept;
int accept(int socket, struct sockaddr *restrict address,
           socklen_t *restrict address_len) {

    real_accept = dlsym(RTLD_NEXT, "accept");

    struct sockaddr_in *myaddr = (struct sockaddr_in *)address;
    int myPort = ntohs(myaddr->sin_port);
    char ipAddress[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(myaddr->sin_addr), ipAddress, INET_ADDRSTRLEN);

    write_log("ACCEPT called sockdf=%d, IP=%s, Port=%d\n", socket, ipAddress,
              myPort);

    connection_socket = real_accept(socket, address, address_len);
    write_log("Accept return on socket %d, setting %d as connection_socket\n",
              socket, connection_socket);

    return connection_socket;
}
#endif

#ifdef ENVOY
#define PORT 8080

typedef int (*connect_t)(int sockfd, const struct sockaddr *addr,
                         socklen_t addrlen);
connect_t real_connect;
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    write_log("Connecting...\n");
    // Helper to load the real library mapping for use
    if (!real_connect) {
        real_connect = dlsym(RTLD_NEXT, "connect");
    }

    write_log("About to call real_connect\n");

    int resp = real_connect(sockfd, addr, addrlen);
    struct sockaddr_in *myaddr = (struct sockaddr_in *)addr;
    int myPort = ntohs(myaddr->sin_port);
    char ipAddress[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(myaddr->sin_addr), ipAddress, INET_ADDRSTRLEN);

    write_log("CONNECT sockfd=%d, IP=%s, Port=%d resp=(%d)\n", sockfd,
              ipAddress, myPort, resp);

    /*
     * It is possible for connect to return -1. From the man pages:
     *
     * The socket is nonblocking and the connection cannot be completed
     * immediately.   It is  possible  to  select(2)  or  poll(2) for completion
     * by selecting the socket for writing.  After select(2) indicates
     * writability,  use  getsockopt(2)  to  read  the SO_ERROR  option  at
     * level SOL_SOCKET to determine whether connect() completed suc- cessfully
     * (SO_ERROR is zero) or unsuccessfully (SO_ERROR is one of the usual  error
     * codes listed here, explaining the reason for the failure).
     */
    if (resp < 0) {
        perror("connect");
    }

    if (myPort == PORT) {
        write_log("CONNECT on target port, connection FD=%d\n", sockfd);
        connection_socket = sockfd;
    }

    return resp;
}
#endif

__attribute__((constructor)) static void setup(void) {
    write_log("Buffer setting up!\n");
#ifdef SERVICE
    sleep(2);
#endif
    buffer_setup();
    write_log("Done setting up\n");
}

// General function wrappers around read/write
typedef ssize_t (*write_t)(int fd, const void *buf, size_t count);
write_t real_write;
ssize_t write(int fd, const void *buf, size_t count) {
    if (!real_write) {
        real_write = dlsym(RTLD_NEXT, "write");
    }

    ssize_t res = 0;
    if (fd == connection_socket && count > 0) {
        res = buffer_write(buf, count);
        write_log("Buffer write returned %zu\n", res);
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

    ssize_t res = 0;
    if (fd == connection_socket) {
        res = buffer_read(buf, count);
        write_log("Buffer Read return %zu \n", res);
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
        resp = buffer_writev(iov, iovcnt);
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
        write_log("BUFFER READV called\n");
        resp = buffer_readv(iov, iovcnt);
        write_log("BUFFER READV returned %zu\n", resp);
        if (resp == 0) {
            write_log("-----------------------------------------\n");
        }
    } else {
        resp = real_readv(fd, iov, iovcnt);
    }
    return resp;
}

typedef int (*epoll_wait_t)(int epfd, struct epoll_event *events, int maxevents,
                            int timeout);
epoll_wait_t real_epoll_wait;
int epoll_wait(int epfd, struct epoll_event *events, int maxevents,
               int timeout) {
    real_epoll_wait = dlsym(RTLD_NEXT, "epoll_wait");
    int ret = real_epoll_wait(epfd, events, maxevents, timeout);

    // assume connection_socket is not in the events
    int conn = 1;

    for (int i = 0; i < ret; i++) {
        if (events[i].data.fd == connection_socket)
            conn = 0;
    }

    // if (buffer_ready() && conn) { /* If the buffer is ready */
    //     events[ret].data.fd = connection_socket;
    //     events[ret].events = 5;
    //     ret++;
    //     // write_log("Manually modifying the entries in epoll_wait\n");
    // }

    return ret;
}

// DEBUGGING , use make debug to compile library with these functions
// TODO: sendmsg is a relatively new syscall and I don't think Python uses
// this...
// TODO: We'll add sendmsg and recvmsg later...

#ifdef DEBUG
typedef ssize_t (*recv_t)(int socket, void *buffer, size_t length, int flags);
recv_t real_recv;
ssize_t recv(int socket, void *buffer, size_t length, int flags) {
    ssize_t ret;

    if (!real_recv)
        real_recv = dlsym(RTLD_NEXT, "recv");

    if (socket == connection_socket) {
        ret = buffer_read(buffer, length);
    } else {
        ret = real_recv(socket, buffer, length, flags);
    }

    return ret;
}

typedef ssize_t (*recvfrom_t)(int socket, void *restrict buffer, size_t length,
                              int flags, struct sockaddr *restrict address,
                              socklen_t *restrict address_len);
recvfrom_t real_recvfrom;
ssize_t recvfrom(int socket, void *restrict buffer, size_t length, int flags,
                 struct sockaddr *restrict address,
                 socklen_t *restrict address_len) {
    ssize_t ret;

    if (!real_recvfrom)
        real_recvfrom = dlsym(RTLD_NEXT, "recvfrom");

    write_log("recvfrom\n");

    if (socket == connection_socket) {
        ret = buffer_read(buffer, length);
    } else {
        ret =
            real_recvfrom(socket, buffer, length, flags, address, address_len);
    }

    return ret;
}

typedef ssize_t (*send_t)(int socket, const void *buffer, size_t length,
                          int flags);
send_t real_send;
ssize_t send(int socket, const void *buffer, size_t length, int flags) {
    ssize_t ret;

    write_log("send\n");

    if (!real_send)
        real_send = dlsym(RTLD_NEXT, "send");

    struct sockaddr_in my_addr;

    memset(&my_addr, 0, sizeof(my_addr));
    socklen_t len = sizeof(my_addr);

    // this looks up which port the addr is bound to,
    // if bind isn't called yet then it returns null
    getsockname(socket, (struct sockaddr *)&my_addr, &len);
    int myPort = ntohs(my_addr.sin_port);
    char ipAddress[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(my_addr.sin_addr), ipAddress, INET_ADDRSTRLEN);

    write_log("SEND sockdf=%d, IP=%s, Port=%d, flags=%d\n", socket, ipAddress,
              myPort, flags);

    if (myPort == SERVICE_PORT) {
        write_log("SERVICE send on TARGET CONNECTION FD=%d\n", socket);
        ret = buffer_write(buffer, length);
        write_log("SERVICE send buffer write on TARGET CONNECTION resp=%zu\n",
                  ret);
    } else {
        ret = real_send(socket, buffer, length, flags);
    }

    return ret;
}

typedef ssize_t (*sendto_t)(int socket, const void *buffer, size_t length,
                            int flags, const struct sockaddr *dest_addr,
                            socklen_t dest_len);
sendto_t real_sendto;
ssize_t sendto(int socket, const void *buffer, size_t length, int flags,
               const struct sockaddr *dest_addr, socklen_t dest_len) {
    ssize_t ret;

    write_log("sendto\n");

    if (!real_sendto)
        real_sendto = dlsym(RTLD_NEXT, "sendto");

    write_log("SERVICE sento on FD=%d\n", socket);
    if (socket == connection_socket) {
        write_log("SERVICE sento on TARGET CONNECTION FD=%d\n", socket);
        ret = buffer_write(buffer, length);
        write_log("SERVICE buffer write on TARGET CONNECTION resp=%zu\n", ret);
    } else {
        ret = real_sendto(socket, buffer, length, flags, dest_addr, dest_len);
    }

    return ret;
}

typedef int (*socket_t)(int domain, int type, int protocol);
socket_t real_socket;
int socket(int domain, int type, int protocol) {
    //    loadReal(real_socket, "socket");
    if (!real_socket) {
        real_socket = dlsym(RTLD_NEXT, "socket");
    }
    int ret = real_socket(domain, type, protocol);
    write_log("called socket (%d, %d, %d) giving back %d \n", domain, type,
              protocol, ret);

    return ret;
}

typedef int (*setsockopt_t)(int socket, int level, int option_name,
                            const void *option_value, socklen_t option_len);
setsockopt_t real_setsockopt;

int setsockopt(int socket, int level, int option_name, const void *option_value,
               socklen_t option_len) {
    if (!real_setsockopt) {
        real_setsockopt = dlsym(RTLD_NEXT, "setsockopt");
    }
    int resp =
        real_setsockopt(socket, level, option_name, option_value, option_len);
    write_log("SETSOCKOPT socket=%d level=%d option_name=%d "
              "option_value=%s option_len=%zd resp=(%d)\n",
              socket, level, option_name, option_value, option_len, resp);
    return resp;
}

typedef int (*socketpair_t)(int domain, int type, int protocol, int sv[2]);
socketpair_t real_socketpair;

int socketpair(int domain, int type, int protocol, int sv[2]) {
    if (!real_socketpair) {
        real_socketpair = dlsym(RTLD_NEXT, "socketpair");
    }
    int resp = real_socketpair(domain, type, protocol, sv);
    write_log("SOCKETPAIR domain=%d type=%d protocol=%d "
              "resp=(%d)\n",
              domain, type, protocol, resp);
    return resp;
}
#endif