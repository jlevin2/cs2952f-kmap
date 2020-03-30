#define _GNU_SOURCE

#include <stdio.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

typedef int (*socket_t)(int domain, int type, int protocol);

typedef int (*bind_t)(int sockfd, const struct sockaddr *addr,
         socklen_t addrlen);

socket_t real_socket;
bind_t real_bind;

int socket(int domain, int type, int protocol) {
    fprintf(stderr, "called socket (%d, %d, %d)\n", domain, type, protocol);
    if (!real_socket) {
        real_socket = dlsym(RTLD_NEXT, "socket");
    }

    return real_socket(domain, type, protocol);
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
    int len = sizeof(my_addr);
    // this looks up which port the addr is bound to,
    // if bind isn't called yet then it returns null
    getsockname(socket, (struct sockaddr *) &my_addr, &len);
    int myPort = ntohs(my_addr.sin_port);
    fprintf(stderr, "Local port : %d\n", myPort);

    return bindFD;
}

// Pretty sure this is unnecessary
__attribute__((constructor)) static void setup(void) {
    fprintf(stderr, "called setup()\n");
}


// socket creation and binding API
// Socket creation -- don't really need to worry about it.
//int bind(int socket, const struct sockaddr *address, socklen_t address_len) {
//    int ret = syscall(SYS_bind, socket, address, address_len);
//
//    struct sockaddr_in my_addr;
//
//    memset(&my_addr, 0, sizeof(my_addr));
//    int len = sizeof(my_addr);
//    getsockname(socket, (struct sockaddr *) &my_addr, &len);
//    int myPort = ntohs(my_addr.sin_port);
//    printf("Local port : %u\n", myPort);
//
//    if (myPort == 8080) {
//        seg();
//    }
//
//    return ret;
//    return syscall(SYS_bind, socket, address, address_len);
//}
//
//ssize_t read(int fd, void *buf, size_t num) {
//    if (fd != special_fd)
//        return syscall(SYS_read, fd, buf, num);
//}
//
//ssize_t write(int fd, const void *buf, size_t num) {
//    if (fd != special_fd)
//        return syscall(SYS_write, fd, buf, num);
//}
//
//
//
