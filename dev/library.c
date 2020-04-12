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

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

#ifdef ENVOY
#define write_log(format,args...) fprintf(stderr, RED format RESET, ## args);
//#define PRINTF(...) printf(BLU __VA_ARGS__ RESET)
#endif

#ifdef SERVICE
#define write_log(format,args...) fprintf(stderr, BLU format RESET, ## args);
//#define PRINTF(...) printf(MAG __VA_ARGS__ RESET)
#endif


// UNIVERSAL
int inbound_socket = 0;
int connection_socket = 0;

int segfault() {
    int *ptr = NULL;
    return *ptr;
}

#ifdef SERVICE
#define SERVICE_PORT 8080
#define SERVICE_ADDRESS "127.0.0.1"

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
    write_log( "Bind called on local port : %d\n", myPort);
    if (myPort == SERVICE_PORT) {
        write_log( "Storing socket=%d binded to port=%d\n", socket, myPort);
        inbound_socket = socket;
    }

    return bindFD;
}

typedef int (*listen_t)(int socket, int backlog);
listen_t real_listen;
int listen(int socket, int backlog) {
    if (!real_listen) {
        real_listen = dlsym(RTLD_NEXT, "listen");
    }
    write_log( "Listen on socket %d\n", socket);

    return real_listen(socket, backlog);
}

typedef int (*socket_t)(int domain, int type, int protocol);
socket_t real_socket;
int socket(int domain, int type, int protocol) {
    real_socket = dlsym(RTLD_NEXT, "socket");
    int ret = real_socket(domain, type, protocol);
    write_log( "New Socket : %d\n", ret);
    if (domain == PF_LOCAL) {
        connection_socket = ret;
    }

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
    write_log( "SETSOCKOPT socket=%d level=%d option_name=%d "
                    "option_value=%s option_len=%zu resp=(%d)\n", socket,
            level, option_name, option_value, option_len, resp);
    return resp;
}

typedef ssize_t (*send_t)(int socket, const void *buffer, size_t length, int flags);
send_t real_send;
ssize_t send(int socket, const void *buffer, size_t length, int flags) {
    ssize_t ret;

    if (!real_send)
        real_send = dlsym(RTLD_NEXT, "send");

    struct sockaddr_in my_addr;

    memset(&my_addr, 0, sizeof(my_addr));
    socklen_t len = sizeof(my_addr);
    // this looks up which port the addr is bound to,
    // if bind isn't called yet then it returns null
    getsockname(socket, (struct sockaddr *) &my_addr, &len);
    int myPort = ntohs(my_addr.sin_port);
    char ipAddress[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(my_addr.sin_addr), ipAddress, INET_ADDRSTRLEN);

    write_log("SEND sockdf=%d, IP=%s, Port=%d, flags=%d\n", socket, ipAddress, myPort, flags);

    if (myPort == SERVICE_PORT) {
        write_log( "SERVICE send on TARGET CONNECTION FD=%d\n", socket);
        ret = buffer_write(buffer, length);
        write_log( "SERVICE send buffer write on TARGET CONNECTION resp=%zu\n", ret);
        ret = real_send(socket, buffer, length, flags);
        write_log( "SERVICE REAL SEND TARGET CONNECTION resp=%zu\n", ret);
    } else {
        ret = real_send(socket, buffer, length, flags);
    }

    return ret;
}

typedef ssize_t (*sendto_t)(int socket, const void *buffer, size_t length, int flags,
                             const struct sockaddr *dest_addr, socklen_t dest_len);
sendto_t real_sendto;
ssize_t sendto(int socket, const void *buffer, size_t length, int flags,
               const struct sockaddr *dest_addr, socklen_t dest_len) {
    ssize_t ret;

    if (!real_sendto)
        real_sendto = dlsym(RTLD_NEXT, "sendto");

    write_log( "SERVICE sento on FD=%d\n", socket);
    if (socket == connection_socket) {
        write_log( "SERVICE sento on TARGET CONNECTION FD=%d\n", socket);
        ret = buffer_write(buffer, length);
        write_log( "SERVICE buffer write on TARGET CONNECTION resp=%zu\n", ret);
        ret = real_sendto(socket, buffer, length, flags, dest_addr, dest_len);
        write_log( "SERVICE REAL SENDTO TARGET CONNECTION resp=%zu\n", ret);
    } else {
        ret = real_sendto(socket, buffer, length, flags, dest_addr, dest_len);
    }

    return ret;
}


#endif

#ifdef ENVOY
#define PORT 8080

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

    write_log("CONNECT sockdf=%d, IP=%s, Port=%d resp=(%d)\n", sockfd, ipAddress, myPort, resp);

//    write_log( "CONNECT sockdf=%d, IP=%s, Port=%d resp=(%d)\n", sockfd, ipAddress, myPort, resp);

    if (resp < 0) {
        perror("Connect returned error code:");
    }
    if (myPort == PORT) {
        write_log( "CONNECT on target port, connection FD=%d\n", sockfd);
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
        write_log( "Buffer write on connection=%d,len=%zd, contents=%s\n", fd,  count, ((char *)buf));
        res = buffer_write(buf, count);
        write_log( "Buffer write returned %zu\n", res);
        res = real_write(fd, buf, count);
        write_log( "REAL write return %zu \n", res);
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
        write_log( "Buffer Read on connection=%d,len=%zd, contents=%s\n", fd,  count, ((char *)buf));
        res = buffer_read(buf, count);
        write_log( "Buffer Read return %zu \n", res);
        res = real_read(fd, buf, count);
        write_log( "REAL Read return %zu \n", res);
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
        write_log( "BUFFER WRITEV on connection FD=%d\n", fildes);
        resp = buffer_writev(iov, iovcnt);
        write_log( "BUFFER WRITEV returned %zu\n", resp);
        resp = real_writev(fildes, iov, iovcnt);
        write_log( "REAL WRITEV returned %zu\n", resp);
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
        write_log( "BUFFER READV on connection FD=%d\n", fd);
        resp = buffer_readv(iov, iovcnt);
        write_log( "BUFFER READV returned %zu\n", resp);
        resp = real_readv(fd, iov, iovcnt);
        write_log( "REAL READV returned %zu\n", resp);
    } else {
        resp = real_readv(fd, iov, iovcnt);
    }
    return resp;
}

// TODO: sendmsg is a relatively new syscall and I don't think Python uses this...
// TODO: We'll add sendmsg and recvmsg later...

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

typedef ssize_t (*recvfrom_t)(int socket, void *restrict buffer, size_t length, int flags,
                              struct sockaddr *restrict address, socklen_t *restrict address_len);
recvfrom_t real_recvfrom;
ssize_t recvfrom(int socket, void *restrict buffer, size_t length, int flags,
                 struct sockaddr *restrict address, socklen_t *restrict address_len){
    ssize_t ret;

    if (!real_recvfrom)
        real_recvfrom = dlsym(RTLD_NEXT, "recv");

    if (socket == connection_socket) {
        ret = buffer_read(buffer, length);
    } else {
        ret = real_recvfrom(socket, buffer, length, flags, address, address_len);
    }

    return ret;
}


// This doesn't work, should initialize if undefined on call (not assuming beforehand)
//__attribute__((constructor)) static void setup(void) {
//    buffer_setup();
//#ifdef SERVICE
//    connection_socket = 4;
//#endif
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
    write_log( "called socket (%d, %d, %d) giving back %d \n", domain, type, protocol, ret);

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
    int resp = real_setsockopt(socket, level, option_name, option_value, option_len);
    write_log( "SETSOCKOPT socket=%d level=%d option_name=%d "
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
    int resp = real_socketpair(domain, type, protocol, sv);
    write_log( "SOCKETPAIR domain=%d type=%d protocol=%d "
                    "resp=(%d)\n", domain, type, protocol, resp);
    return resp;
}
#endif