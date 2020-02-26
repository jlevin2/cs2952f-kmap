#include <unistd.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>

int special_fd = -1;

int seg() {
    int *ptr = NULL;
    return *ptr;
}

// socket creation and binding API
// Socket creation -- don't really need to worry about it.
int bind(int socket, const struct sockaddr *address, socklen_t address_len) {
    int ret = syscall(SYS_bind, socket, address, address_len);

    struct sockaddr_in my_addr;

    memset(&my_addr, 0, sizeof(my_addr));
    int len = sizeof(my_addr);
    getsockname(socket, (struct sockaddr *) &my_addr, &len);
    int myPort = ntohs(my_addr.sin_port);
    printf("Local port : %u\n", myPort);

    if (myPort == 8080) {
        seg();
    }

    return ret;
}

ssize_t read(int fd, void *buf, size_t num) {
    if (fd != special_fd)
        return syscall(SYS_read, fd, buf, num);
}

ssize_t write(int fd, const void *buf, size_t num) {
    if (fd != special_fd)
        return syscall(SYS_write, fd, buf, num);
}



