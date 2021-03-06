#ifndef SOCKET_H
#define SOCKET_H
/**
 * @file socket.h
 * @brief POSIX-compatible socket library supporting TCP protocol on IPv4. 
 */
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netdb.h>

#include "tcp.h"


#define SOCKET_INUSE 1
#define SOCKET_BOUND 2


#define maxBacklog 20
#define maxSocketSize 20


struct myOwnSocket {
    int permBits;

    int domain;
    int type;
    int protocol;

    pthread_mutex_t lock;

    int backlog;
    
    uint16_t srcport;
    uint32_t srcaddr;

    // struct tcpInfo *que[maxBacklog]; // tcp connections.
    int que[maxBacklog]; // store the tcp connection descriptor.
};

struct myOwnSocket sock[maxSocketSize];


/**
 * @see [POSIX.1-2017:socket](http://pubs.opengroup.org/onlinepubs/9699919799/functions/socket.html)
 */
int __wrap_socket(int domain, int type, int protocol);


/**
 * @see [POSIX.1-2017:bind](http://pubs.opengroup.org/onlinepubs/9699919799/functions/bind.html)
 */
int __wrap_bind(int socket, const struct sockaddr *address, socklen_t address_len);


/**
 * @see [POSIX.1-2017:listen](http://pubs.opengroup.org/onlinepubs/9699919799/functions/listen.html)
 */
int __wrap_listen(int socket, int backlog);


/**
 * @see [POSIX.1-2017:connect](http://pubs.opengroup.org/onlinepubs/9699919799/functions/connect.html)
 */
int __wrap_connect(int socket , const struct sockaddr *address , socklen_t address_len);


/**
 * @see [POSIX.1-2017:accept](http://pubs.opengroup.org/onlinepubs/9699919799/functions/accept.html)
 */
int __wrap_accept(int socket , struct sockaddr *address , socklen_t *address_len);


/**
 * @see [POSIX.1-2017:read](http://pubs.opengroup.org/onlinepubs/9699919799/functions/read.html)
 */
ssize_t __wrap_read(int fildes, void *buf, size_t nbyte);


/**
 * @see [POSIX.1-2017:write](http://pubs.opengroup.org/onlinepubs/9699919799/functions/write.html)
 */
ssize_t __wrap_write(int fildes, const void *buf, size_t nbyte);


/**
 * @see [POSIX.1-2017:close](http://pubs.opengroup.org/onlinepubs/9699919799/functions/close.html)
 */
int __wrap_close(int fildes);


/**
 * @see [POSIX.1-2017:getaddrinfo](http://pubs.opengroup.org/onlinepubs/9699919799/functions/getaddrinfo.html)
 */
int __wrap_getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);

#endif // SOCKET_H