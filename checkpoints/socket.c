
#include "socket.h"

void* Socket_packetReceiver(void *p) {
    int device = (int)p;
    setFrameReceiveCallback(frameCallbackExample, device); // 监听device收包.
}

void socketInit() {
    pthread_mutex_init(&printfMutex, NULL);
    pthread_mutex_init(&setRoutingTableLock, NULL);
    pthread_mutex_init(&edgeEntryLock, NULL);

    addDevices();
    setIPPacketReceiveCallback(ipCallbackExample);
    setTCPPacketReceiveCallback(TCPCallback);


    pthread_t packet_sender;
    pthread_t packet_receiver[10];

    // sync_printf("device number: %d\n", device_num);
    for(int i = 0; i < device_num; ++i) {
        if (device_list[i] == NULL) continue;
        pthread_create(packet_receiver + i, NULL, Socket_packetReceiver, (void *)(i));
    }
}

/* create an unbound socket.
 * return file descriptor for later communication with sockets.
 * *** allocate the lowest numbered available
 * domain:  AF_INET:Address Family  PF_INET:Protocol Family ......
 * type: SOCK_STREAM(TCP) SOCK_DGRAM SOCK_SEQPACKET ......
 * protocol: IPPROTO_TCP IPPROTO_UDP IPPROTO_ICMP ......
 * type matches with protocol.
 */
int __wrap_socket(int domain, int type, int protocol) {
    static int initial = 0;
    if (initial == 0) {
        socketInit();
        initial = 1; // ugly... but I don't know how to realize it...
    }

    // the undefined function go to system's API.
    if (domain != AF_INET || type != SOCK_STREAM || (protocol > 0 && protocol != IPPROTO_TCP))
        return socket(domain, type, protocol);
    int sockNumber = -1;
    for (int i = 0; i < maxSocketSize; ++i) if (!(sock[i].permBits & SOCKET_INUSE)) {
        sockNumber = i;
        break;
    }
    if (sockNumber == -1) return printf("Creating socket wrong!\n"), sockNumber;

    int idx = allocSocketDescriptor(1, &sock[sockNumber]);
    // printf("socket: %d %d\n", sockNumber, idx);
    if (idx == -1) return -1;


    sock[sockNumber].permBits |= SOCKET_INUSE;
    sock[sockNumber].domain = domain;
    sock[sockNumber].type = type;
    sock[sockNumber].protocol = protocol;
    pthread_mutex_init(&sock[sockNumber].lock, NULL);

    return idx;
}

/* socket: Specifies the file descriptor of the socket to be bound.
 * address: Points to a sockaddr structure containing the address to be bound to the socket. The length and format of the address depend on the address family of the socket.
 * address_len: Specifies the length of the sockaddr structure pointed to by the address argument.
 */
int __wrap_bind(int socket, const struct sockaddr *address, socklen_t address_len) {
    int sockNumber = maintainSocketDescriptor(socket);
    // printf("bind: %d %d\n", socket, sockNumber);
    // while(1);
    if (sockNumber == -1 || address -> sa_family != AF_INET) // if not for me, go to system.
        return bind(socket, address, address_len);
    if (mapSocket[sockNumber].type != 1) return -1;

    struct myOwnSocket *p = (struct myOwnSocket *)mapSocket[sockNumber].addr;
    if ((p -> permBits & SOCKET_INUSE) == 0) return -1; // not in use.
    if ((p -> permBits & SOCKET_BOUND) != 0) return -1; // already bound.

    p -> permBits |= SOCKET_BOUND;
    p -> srcaddr = ((struct sockaddr_in *)address) -> sin_addr.s_addr;
    if (p -> srcaddr == 0) { // find an available IP address.
        for (int i = 0; i < device_num; ++i) {
            int t = 0;
            for (int j = 0; j < maxSocketSize; ++j) if (sock[i].permBits & SOCKET_INUSE) {
                t |= sock[i].srcaddr == device_list[i] -> ip;
            }
            if (!t) {
                p -> srcaddr = device_list[i] -> ip;
                break;
            }
        }
    }
    p -> srcport = ((struct sockaddr_in *)address) -> sin_port;
    return 0;
}


// mark a connection-mode socket, specified by the socket argument, as accepting connections.
int __wrap_listen(int socket, int backlog) {
    backlog = min(backlog, maxBacklog); // my design flaw...
    int sockNumber = maintainSocketDescriptor(socket);
    // printf("listen: %d %d\n", socket, sockNumber);
    if (sockNumber == -1) return listen(socket, backlog);
    if (mapSocket[sockNumber].type != 1) return -1;


    struct myOwnSocket *p = (struct myOwnSocket *)mapSocket[sockNumber].addr;
    if ((p -> permBits & SOCKET_BOUND) == 0) return -1; // unbound.

    struct tcpHeader hd;

    hd.src.s_addr = p -> srcaddr;
    hd.srcport = p -> srcport;
    hd.dst.s_addr = 0;
    hd.dstport = 0;
    
    p -> backlog = backlog;
    for (int i = 0; i < backlog; ++i) {
        p -> que[i] = allocSocketDescriptor(2, initiateTCPConnect(hd, LISTEN));
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
    // printf("connect: %d %d\n", socket, sockNumber);
    if (sockNumber == -1) return connect(socket, address, address_len);
    if (mapSocket[sockNumber].type != 1) return -1;
    struct myOwnSocket *p = (struct myOwnSocket *)mapSocket[sockNumber].addr;

    struct tcpHeader hd;
    hd.dstport = ((struct sockaddr_in *)address) -> sin_port;
    hd.dst.s_addr = ((struct sockaddr_in *)address) -> sin_addr.s_addr;
    hd.srcport = rand() %32768 + 32768; // asd123www: source port 是什么?
    // printf("%d\n", device_list[0] == NULL);
    hd.src.s_addr = device_list[0] -> ip; // select a arbitrary source ip address.

    struct tcpInfo *addr = initiateTCPConnect(hd, TCP_SYN_SENT);
    if (addr == NULL) return -1;

    // free the original socket, cause later we'll only use the tcp.
    memset(mapSocket[sockNumber].addr, 0, sizeof(struct myOwnSocket));
    mapSocket[sockNumber].type = 2;
    mapSocket[sockNumber].addr = addr; // redirect to the tcp connection.

    pthread_t individualTCPSender;
    pthread_create(&individualTCPSender, NULL, tcpSender, addr);

    int times = 20000;
    while (times--) {
        if (addr -> tcb.tcpState == TCP_ESTABLISHED) return 0;
        usleep(1e3); // wait for some time.
    }

    return -1; // time-out.
}


/*
 * socket: Specifies a socket that was created with socket(), has been bound to an address with bind(), and has issued a successful call to listen().
 * address: Either a null pointer, or a pointer to a sockaddr structure where the address of the connecting socket shall be returned.
 * address_len: Either a null pointer, if address is a null pointer, or a pointer to a socklen_t object which on input specifies the length of the supplied sockaddr structure, and on output specifies the length of the stored address.
 */
int __wrap_accept(int socket , struct sockaddr *address , socklen_t *address_len) {
    int sockNumber = maintainSocketDescriptor(socket);
    // printf("accept: %d %d\n", socket, sockNumber);
    if (socket == -1) return accept(socket, address, address_len);
    if (mapSocket[sockNumber].type != 1) return -1; // not socket.

    int pos = -1;
    struct myOwnSocket *p = (struct myOwnSocket *)mapSocket[sockNumber].addr;
    if ((p -> permBits & SOCKET_BOUND) == 0) return -1;

    pthread_mutex_lock(&p -> lock); // syn for require a tcp connection.
    // you should block until one found.
    // printf("bool: %d\n", *address_len);

    while (1) {
        // printf("hello, I'm finding\n");
        for (int i = 0; i < p -> backlog; ++i) {
            int idx = maintainSocketDescriptor(p -> que[i]);
            if (idx == -1) {
                p -> que[i] = 0;
                continue;
            }
            // printf("%d %d\n",mapSocket[idx].type,  ((struct tcpInfo *)mapSocket[idx].addr) -> tcb.tcpState);
            if (mapSocket[idx].type == 2 && ((struct tcpInfo *)mapSocket[idx].addr) -> tcb.tcpState == TCP_ESTABLISHED){ // found one connect.
                if (address != NULL) { // we should store the address in it.
                    address -> sa_family = AF_INET;
                    ((struct sockaddr_in *)address) -> sin_addr.s_addr = ((struct tcpInfo *)mapSocket[idx].addr) -> h -> dst.s_addr;
                    ((struct sockaddr_in *)address) -> sin_port = ((struct tcpInfo *)mapSocket[idx].addr) -> h -> dstport;
                }
                int tmp = p -> que[i];
                p -> que[i] = 0; //delete it, so next accepts will not get the same tcp connect.
                pthread_mutex_unlock(&p -> lock);

                pthread_t individualTCPSender;
                pthread_create(&individualTCPSender, NULL, tcpSender, ((struct tcpInfo *)mapSocket[idx].addr)); // you must have a sender thread...

                return tmp;
            }
        }
        // usleep(1e3); // sleep for some time...
    }

    pthread_mutex_unlock(&p -> lock);
    return -1;
}

// block until data available.
ssize_t __wrap_read(int fildes, void *buf, size_t nbyte) {
    if (nbyte == 0) return -1;
    int sockNumber = maintainSocketDescriptor(fildes);
    if (sockNumber == -1) return read(fildes, buf, nbyte);
    if (mapSocket[sockNumber].type != 2) return -1; //not write to the tcp.
    struct tcpInfo *tcp = (struct tcpInfo *)mapSocket[sockNumber].addr;
    // int times = 500; // times/100 seconds.
    while (1) {
        int size = ringBufferSize(tcp -> rx);
        if (size) {
            int state = fetchTCPData(tcp, buf, min(size, nbyte));
            // printf("read %d\n", state);
            if (state != -1) return state;
        }
        if (tcp -> tcb.tcpState != TCP_ESTABLISHED) break;
        // usleep(1e3);
    }
    return 0; // fetchTCPData(tcp, buf, nbyte);
}

ssize_t __wrap_write(int fildes, const void *buf, size_t nbyte) {
    int sockNumber = maintainSocketDescriptor(fildes);
    // printf("write: %d %d\n", fildes, mapSocket[sockNumber].type);
    if (sockNumber == -1) return write(fildes, buf, nbyte);
    if (mapSocket[sockNumber].type != 2) return -1; //not write to the tcp.

    // printf("write into buffer %ld\n", nbyte);

    struct tcpInfo *tcp = (struct tcpInfo *)mapSocket[sockNumber].addr;
    return pushTCPData(tcp, buf, nbyte);
}

// deallocate the file descriptor indicated by fildes
int __wrap_close(int fildes) {
    int sockNumber = maintainSocketDescriptor(fildes);
    if (sockNumber == -1) return close(fildes);

    if (mapSocket[sockNumber].type == 2) { // tcp connection.
        // connection oriented, wait for data to be transmitted.


        while (ringBufferSize(((struct tcpInfo *)mapSocket[sockNumber].addr) -> tx) != 0) {
            // usleep(1e3);
        }

        ((struct tcpInfo *)mapSocket[sockNumber].addr) -> wantClose = 1;

        while (((struct tcpInfo *)mapSocket[sockNumber].addr) -> tcb.tcpState != TCP_CLOSE) {
            // usleep(1e3);
        }

        freeTCPConnect((struct tcpInfo *)mapSocket[sockNumber].addr);
        mapSocket[sockNumber].type = 0;
        mapSocket[sockNumber].addr = NULL;
    }
    else { // socket.
        memset(mapSocket[sockNumber].addr, 0, sizeof(struct myOwnSocket)); 
        mapSocket[sockNumber].type = 0;
        mapSocket[sockNumber].addr = NULL;
    }
    return 0;
}

int __wrap_getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res) {
    return getaddrinfo(node, service, hints, res);
}