#include <unistd.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>

int special_fd = -1;

int segfault() {
    int *ptr = NULL;
    return *ptr;
}

// socket creation and binding API
// Socket creation -- don't really need to worry about it.
int bind(int socket, const struct sockaddr *address, socklen_t address_len) {
    struct sockaddr_in *sock = (struct sockaddr_in *)address;
    int port = ntohs(sock->sin_port);

    if (port == 80) {
//        segfault();
    }

    return syscall(SYS_bind, socket, address, address_len);
}

ssize_t read(int fd, void *buf, size_t num) {
    return syscall(SYS_read, fd, buf, num);
}

ssize_t write(int fd, const void *buf, size_t num) {
    return syscall(SYS_write, fd, buf, num);
}



