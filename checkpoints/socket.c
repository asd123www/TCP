


#include "socket.h"


/* create an unbound socket.
 * return file descriptor for later communication with sockets.
 * *** allocate the lowest numbered available
 * domain:  AF_INET:Address Family  PF_INET:Protocol Family ......
 * type: SOCK_STREAM(TCP) SOCK_DGRAM SOCK_SEQPACKET ......
 * protocol: IPPROTO_TCP IPPROTO_UDP IPPROTO_ICMP ......
 * type matches with protocol.
 */
int __wrap_socket(int domain, int type, int protocol) {
    // the undefined function go to system's API.
    if (domain != AF_INET || type != SOCK_STREAM || (protocol > 0 && protocol != IPPROTO_TCP))
        return socket(domain, type, protocol);

    int sockNumber = findLowestSocketNumber();
    if (sockNumber == -1) return printf("Creating socket wrong!\n"), sockNumber;

    struct tcpInfo *a = (struct tcpInfo *)malloc(sizeof(struct tcpInfo));
    // asd123www 需要继续进行初始化.
    tcps[sockNumber] = a;

    return mySocketNumberOffset + sockNumber;
}

/* socket: Specifies the file descriptor of the socket to be bound.
 * address: Points to a sockaddr structure containing the address to be bound to the socket. The length and format of the address depend on the address family of the socket.
 * address_len: Specifies the length of the sockaddr structure pointed to by the address argument.
 */
int __wrap_bind(int socket, const struct sockaddr *address, socklen_t address_len) {
    if (socket < mySocketNumberOffset || !checkSocketNumberValid(socket)) return -1;

}



int __wrap_listen(int socket, int backlog) {

}


int __wrap_connect(int socket , const struct sockaddr *address , socklen_t address_len) {

}



int __wrap_accept(int socket , struct sockaddr *address , socklen_t *address_len) {

}


ssize_t __wrap_read(int fildes, void *buf, size_t nbyte) {

}


ssize_t __wrap_write(int fildes, const void *buf, size_t nbyte) {

}


int __wrap_close(int fildes) {


}


int __wrap_getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res) {


}