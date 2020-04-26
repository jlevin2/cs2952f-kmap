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

typedef int (*readv_t)(int fd, const struct iovec *iov, int iovcnt);
readv_t real_readv;

typedef int (*writev_t)(int fildes, const struct iovec *iov, int iovcnt);
writev_t real_writev;

typedef ssize_t (*read_t)(int fd, void *buf, size_t count);
read_t real_read;

typedef ssize_t (*write_t)(int fd, const void *buf, size_t count);
write_t real_write;

#ifdef ENVOY
typedef int (*connect_t)(int sockfd, const struct sockaddr *addr,
                         socklen_t addrlen);
connect_t real_connect;
#endif

#ifdef SERVICE
typedef int (*accept_t)(int socket, struct sockaddr *restrict address,
                        socklen_t *restrict address_len);
accept_t real_accept;
#endif