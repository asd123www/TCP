


#include "socket.h"


// 加个锁...

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
    int sockNumber = -1;
    for (int i = 0; i < maxSocketSize; ++i) if (!(sock[i].permBits & SOCKET_INUSE)) {
        sockNumber = i;
        break;
    }
    if (sockNumber == -1) return printf("Creating socket wrong!\n"), sockNumber;

    int idx = allocSocketDescriptor(0, &sock[sockNumber]);
    if (idx == -1) return -1;


    sock[sockNumber].permBits |= SOCKET_INUSE;
    sock[sockNumber].domain = domain;
    sock[sockNumber].type = type;
    sock[sockNumber].protocol = protocol;

    return idx;
}

/* socket: Specifies the file descriptor of the socket to be bound.
 * address: Points to a sockaddr structure containing the address to be bound to the socket. The length and format of the address depend on the address family of the socket.
 * address_len: Specifies the length of the sockaddr structure pointed to by the address argument.
 */
int __wrap_bind(int socket, const struct sockaddr *address, socklen_t address_len) {
    int sockNumber = maintainSocketDescriptor(socket);
    if (sockNumber == -1 || address -> sa_family != AF_INET) // if not for me, go to system.
        return bind(socket, address, address_len);
    if (mapSocket[sockNumber].type != 0) return -1;

    struct myOwnSocket *p = (struct myOwnSocket *)mapSocket[sockNumber].addr;
    if ((p -> permBits & SOCKET_INUSE) == 0) return -1; // not in use.
    if ((p -> permBits & SOCKET_BOUND) != 0) return -1; // already bound.

    p -> permBits |= SOCKET_BOUND;
    p -> srcaddr = ((struct sockaddr_in *)address) -> sin_addr.s_addr;
    p -> srcport = ((struct sockaddr_in *)address) -> sin_port;

    return 0;
}


// mark a connection-mode socket, specified by the socket argument, as accepting connections.
int __wrap_listen(int socket, int backlog) {
    int sockNumber = maintainSocketDescriptor(socket);
    if (sockNumber == -1) return listen(socket, backlog);
    if (mapSocket[sockNumber].type != 0) return -1;

    struct myOwnSocket *p = (struct myOwnSocket *)mapSocket[sockNumber].addr;
    if ((p -> permBits & SOCKET_BOUND) == 0) return -1; // unbound.

    struct tcpHeader hd;

    hd.src.s_addr = p -> srcaddr;
    hd.dst.s_addr = 0;

    hd.srcport = p -> srcport;
    hd.dstport = 0;
    
    p -> backlog = backlog;
    for (int i = 0; i < backlog; ++i) {
        p -> que[i] = allocSocketDescriptor(1, initiateTCPConnect(hd, LISTEN));
    }
    return 0;
}

/*
 * socket: Specifies the file descriptor associated with the socket.
 * address: Points to a sockaddr structure containing the peer address. The length and format of the address depend on the address family of the socket.
 * address_len: Specifies the length of the sockaddr structure pointed to by the address argument.
*/
int __wrap_connect(int socket , const struct sockaddr *address , socklen_t address_len) {
    int sockNumber = maintainSocketDescriptor(socket);
    if (sockNumber == -1) return connect(socket, address, address_len);
    if (mapSocket[sockNumber].type != 0) return -1;

    struct myOwnSocket *p = (struct myOwnSocket *)mapSocket[sockNumber].addr;

    struct tcpHeader hd;
    hd.dstport = ((struct sockaddr_in *)address) -> sin_port;
    hd.dst.s_addr = ((struct sockaddr_in *)address) -> sin_addr.s_addr;
    hd.srcport = rand(); // asd123www: source port 是什么?
    hd.src.s_addr = -1;
    hd.src.s_addr = subnet_list[findIPAddress(hd.dst, hd.src)] -> addr;

    p -> que[0] = allocSocketDescriptor(1, initiateTCPConnect(hd, TCP_SYN_SENT));
    if (p -> que[0] == -1) return -1;

    pthread_t individualTCPSender;
    pthread_create(&individualTCPSender, NULL, tcpSender, mapSocket[maintainSocketDescriptor(p -> que[0])].addr);

    return 0;
}


/*
 * socket: Specifies a socket that was created with socket(), has been bound to an address with bind(), and has issued a successful call to listen().
 * address: Either a null pointer, or a pointer to a sockaddr structure where the address of the connecting socket shall be returned.
 * address_len: Either a null pointer, if address is a null pointer, or a pointer to a socklen_t object which on input specifies the length of the supplied sockaddr structure, and on output specifies the length of the stored address.
 */
int __wrap_accept(int socket , struct sockaddr *address , socklen_t *address_len) {
    int sockNumber = maintainSocketDescriptor(socket);
    if (socket == -1) return accept(socket, address, address_len);
    if (mapSocket[sockNumber].type != 0) return -1; // not socket.

    int pos = -1;
    struct myOwnSocket *p = (struct myOwnSocket *)mapSocket[sockNumber].addr;
    if ((p -> permBits & SOCKET_BOUND) == 0) return -1;
    for (int i = 0; i < p -> backlog; ++i) {
        int idx = maintainSocketDescriptor(p -> que[i]);
        if (idx == -1) {
            p -> que[i] = 0;
            continue;
        }
        if (mapSocket[idx].type == 1 && ((address == NULL) || 
            (((struct tcpInfo *)mapSocket[idx].addr) -> h -> dst.s_addr == ((struct sockaddr_in *)address) -> sin_addr.s_addr &&
             ((struct tcpInfo *)mapSocket[idx].addr) -> h -> dstport == ((struct sockaddr_in *)address) -> sin_port))) 
                return p -> que[i];
    }
    return -1;
}


ssize_t __wrap_read(int fildes, void *buf, size_t nbyte) {
    int sockNumber = maintainSocketDescriptor(fildes);
    if (sockNumber == -1) return read(fildes, buf, nbyte);
    if (mapSocket[sockNumber].type != 1) return -1; //not write to the tcp.

    struct tcpInfo *tcp = (struct tcpInfo *)mapSocket[sockNumber].addr;
    return fetchTCPData(tcp, buf, nbyte);
}

ssize_t __wrap_write(int fildes, const void *buf, size_t nbyte) {
    int sockNumber = maintainSocketDescriptor(fildes);
    if (sockNumber == -1) return write(fildes, buf, nbyte);
    if (mapSocket[sockNumber].type != 1) return -1; //not write to the tcp.

    struct tcpInfo *tcp = (struct tcpInfo *)mapSocket[sockNumber].addr;
    return pushTCPData(tcp, buf, nbyte);
}


// deallocate the file descriptor indicated by fildes
int __wrap_close(int fildes) {
    int sockNumber = maintainSocketDescriptor(fildes);
    if (sockNumber == -1) return close(fildes);

    if (mapSocket[sockNumber].type == 1) { // tcp connection.
        freeTCPConnect((struct tcpInfo *)mapSocket[sockNumber].addr);
        mapSocket[sockNumber].type = -1;
        mapSocket[sockNumber].addr = NULL;
    }
    else { // socket.
        memset(mapSocket[sockNumber].addr, 0, sizeof(struct myOwnSocket)); 
        mapSocket[sockNumber].type = -1;
        mapSocket[sockNumber].addr = NULL;
    }
    return 0;
}

int __wrap_getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res) {
    return getaddrinfo(node, service, hints, res);
}