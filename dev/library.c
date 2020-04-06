#define _GNU_SOURCE

#include "fifo.h"

#include <arpa/inet.h>
#include <string.h>
#include <dlfcn.h>


typedef int (*socket_t)(int domain, int type, int protocol);

typedef int (*bind_t)(int sockfd, const struct sockaddr *addr,
         socklen_t addrlen);

socket_t real_socket;
bind_t real_bind;

int target_socket = 0;
int cur_accepted_socket = 0;

#define PORT 80

int socket(int domain, int type, int protocol) {

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

typedef int (*accept_t)(int socket, struct sockaddr *address, socklen_t *address_len);


accept_t real_accept;

int accept(int socket, struct sockaddr *address, socklen_t *address_len) {
    if (!real_accept) {
        real_accept = dlsym(RTLD_NEXT, "accept");
    }
    fprintf(stderr, "Accept on socket %d\n", socket);
    int returnFD = real_accept(socket, address, address_len);
    if (socket == target_socket) {
        fprintf(stderr, "Target socket accepted new connection on %d!\n", returnFD);
        cur_accepted_socket = returnFD;
    }
    return returnFD;
}

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
    fprintf(stderr, "Write on socket=%d,len=%zu, contents=%s\n", fd,count, ((char *)buf));
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
    fprintf(stderr, "READ on socket=%d,len=%zu, contents=%s\n", fd,count, ((char *)buf));
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


// Pretty sure this is unnecessary
//__attribute__((constructor)) static void setup(void) {
//    fifo_setup(); // set up the fifo
//    fprintf(stderr, "called setup()\n");
//}


