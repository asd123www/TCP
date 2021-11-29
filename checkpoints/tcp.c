
#include "tcp.h"


/* @brief: alloc smallest free identifier.
 *
 * @param type: tcpInfo or Socket.
 * @param addr: pointer to the struct.
 * return > 0 socket number if succeed, otherwise -1.
 */ 
int allocSocketDescriptor(int type, void *addr) {
    int pos = -1;
    for (int i = 0; i < 2 * MaxTCPNumber; ++i) {
        if (mapSocket[i].type == -1) {
            pos = i;
            break;
        }
    }
    if (pos == -1) return pos;
    mapSocket[pos].type = type;
    mapSocket[pos].addr = addr;
    return pos + mySocketNumberOffset;
}

/* @brief: transfer the socket number to real index.
 *
 * @param type: tcpInfo or Socket.
 * @param addr: pointer to the struct.
 * return the >= 0 real index if succeed, otherwise -1.
 */ 
int maintainSocketDescriptor(int fd) {
    if (fd < mySocketNumberOffset || fd >= mySocketNumberOffset + 2*MaxTCPNumber) return -1;

    int sockNumber = fd - mySocketNumberOffset;
    if (mapSocket[sockNumber].type == -1) return -1;

    return sockNumber;
}

/*
struct tcpInfo {
    pthread_mutex_t lock; // syn for send/receive.

    uint64_t lastClock;
    uint64_t RTT;

    struct tcpHeader *h;

    struct ringBuffer *rx;
    struct ringBuffer *tx;

    struct TransmissionControlBlock tcb;
};*/
struct tcpInfo *initiateTCPConnect(struct tcpHeader hd, int state) {
    struct tcpInfo *tcp = (struct tcpInfo *)malloc(sizeof(struct tcpInfo));
    if (tcp == NULL) {
        printf("in tcp.c/initiateTCPConnect() malloc failed.\n");
        exit(0);
    }
    pthread_mutex_init(&tcp -> lock, NULL);
    tcp -> lastClock = 0;
    tcp -> RTT = CLOCKS_PER_SEC * 0.01; // 1.5s

    tcp -> h = (struct tcpHeader *)malloc(sizeof(struct tcpHeader));
    *(tcp -> h) = hd;

    tcp -> rx = initRingBuffer();
    tcp -> tx = initRingBuffer();

    tcp -> wantClose = 0;

    // asd123www: maybe listen or sendsyn? must be these two types.
    tcp -> tcb.tcpState = state; // the caller define the initial type.
    tcp -> tcb.SND.WND = INITIAL_WINDOW_SIZE; // initiate a window size.
    tcp -> tcb.SND.ISS = clock(); // maybe 64-bit? just module 2^32.
    tcp -> tcb.SND.NXT = tcp -> tcb.SND.UNA = tcp -> tcb.SND.ISS;
    // asd123www: I don't know the meaning of UP or WL1 or WL2.


    // RCV.IRS and RCV.NXT don't know before handshake.
    tcp -> tcb.RCV.WND = INITIAL_WINDOW_SIZE; // It's decided by me, so just be a constant.

    for (int i = 0; i < MaxTCPNumber; ++ i) if (tcps[i] == NULL) {
        tcps[i] = tcp;
        break;
    }

    return tcp;
}

void freeTCPConnect(struct tcpInfo *tcp) {
    if (tcp == NULL) return;
    
    for (int i = 0; i < MaxTCPNumber; ++ i) if (tcps[i] == tcp) tcps[i] = NULL;
    deleteTCPQuintuple(tcp);

    free(tcp -> h);
    freeRingBuffer(tcp -> rx);
    freeRingBuffer(tcp -> tx);
    free(tcp);
    return;
}

int deleteTCPQuintuple(struct tcpInfo *tcp) {
    // we shouldn't find the pointer after free.
    for (int i = 0; i < MaxTCPNumber; ++i) if (tcps[i] == tcp) tcps[i] = NULL;
}

struct tcpInfo *findTCPQuintuple(struct in_addr src, struct in_addr dst, uint16_t srcport, uint16_t dstport) {
    for (int i = 0; i < MaxTCPNumber; ++i) {
        struct tcpInfo *tcp = tcps[i];
        if (tcp == NULL) continue;

        // asd123www: 如果是listen状态就可以创建多个实例?
        // listen需要匹配则在这里更新tcpHeader.

        // printf("port: %d %d %d %d\n", srcport, dstport, tcp -> h -> srcport, tcp -> h -> dstport);
        // printf("port: %d %d %d %d\n", src.s_addr, dst.s_addr, tcp -> h -> src.s_addr, tcp -> h -> dst.s_addr);

        if (tcp -> h -> dstport == dstport && tcp -> h -> srcport == srcport &&
            tcp -> h -> src.s_addr == src.s_addr && tcp -> h -> dst.s_addr == dst.s_addr) return tcp;
        
        if (tcp -> tcb.tcpState == LISTEN && tcp -> h -> src.s_addr == src.s_addr && tcp -> h -> srcport == srcport) {
            tcp -> h -> dstport = dstport;
            tcp -> h -> dst.s_addr = dst.s_addr;
            return tcp;
        }
    }
    return NULL;
}


int convertInt16PC2Net(uint32_t x) {
    return ((x & 255) << 8) | (x >> 8 & 255);
}

int convertInt32PC2Net(uint32_t x) {
    return (convertInt16PC2Net(x & 65535) << 16) | convertInt16PC2Net(x >> 16 & 65535);
}

uint16_t TCPchecksum(const void *buf, int len) {
    uint16_t ans = 0;
    uint16_t *begin = (uint16_t *)buf;
    for (int i = 0; i < len/2; ++i) ans += *(begin + i);
    if (len & 1) ans += (*((uint8_t *)buf + len - 1)); // << 8;
    return ans;
}



int __wrap_pushTCPData(struct tcpInfo *fd, const u_char *buf, int len) {
    if (fd == NULL) return -1;
    int state = ringBufferReceiveSegment(fd -> tx, buf, len);
    return state;
}

int pushTCPData(struct tcpInfo *fd, const u_char *buf, int len) {
    pthread_mutex_lock(&fd -> lock);
    int state = __wrap_pushTCPData(fd, buf, len);
    pthread_mutex_unlock(&fd -> lock);
    return state == 0? len: -1;
}



int __wrap_fetchTCPData(struct tcpInfo *fd, u_char *buf, int len) {
    if (fd == NULL) return -1; // bad tcp.
    u_char *buffer = ringBufferSendSegment(fd -> rx, len, 0);
    if (buffer == NULL) return -1;
    memcpy(buf, buffer, len);
    free(buffer);
    updateRingBufferHead(fd -> rx, len);
    return 0;
}

int fetchTCPData(struct tcpInfo *fd, u_char *buf, int len) {
    pthread_mutex_lock(&fd -> lock);
    int state = __wrap_fetchTCPData(fd, buf, len);
    pthread_mutex_unlock(&fd -> lock);
    return state == 0? len: -1;
}



// /*
// // TCP states
// enum TCPstate{
//     LISTEN = 0, 
//     TCP_SYN_SENT, // sent a connection request, waiting for ack
//     TCP_SYN_RECV,     // received a connection request, sent ack, waiting for final ack in three-way handshake.
//     TCP_ESTABLISHED,  // connection established
//     TCP_FIN_WAIT1,    // our side has shutdown, waiting to complete transmission of remaining buffered data
//     TCP_FIN_WAIT2,	  // all buffered data sent, waiting for remote to shutdown
//     TCP_CLOSING,	  // both sides have shutdown but we still have data we have to finish sending
//     TCP_TIME_WAIT,	  // timeout to catch resent junk before entering closed, can only be entered from FIN_WAIT2
//                       // or CLOSING.  Required because the other end may not have gotten our last ACK causing it
//                       // to retransmit the data packet (which we ignore)
//     TCP_CLOSE_WAIT,   // remote side has shutdown and is waiting for us to finish writing our data and to shutdown (we have to close() to move on to LAST_ACK)
//     TCP_LAST_ACK,	  // out side has shutdown after remote has shutdown.  There may still be data in our buffer that we have to finish sending
//     TCP_CLOSE		  // socket is finished
// };*/
// void initialMatStateAction() {
//     // action: create TCB/ delete TCB not implemented.
//     MatStateAction[TCP_CLOSE][(TCP_SYN) << 1 | ACTION_SEND];
//     MatStateAction[LISTEN][(TCP_SYN) << 1 | ACTION_SEND];
//     MatStateAction[LISTEN][(TCP_SYN | TCP_ACK) << 1 | ACTION_SEND];

// }

// // sender/receiver call, they have syn_lock so we don't have to lock again.
// int TCPStateTransferWithAction(struct TransmissionControlBlock *tcb, int direction, int permBits) {
//     if (tcb == NULL) return -1;
//     int state = tcb -> tcpState;
//     tcb -> tcpState = MatStateAction[tcb -> tcpState][param << 1 | direction];
//     if (tcb -> tcpState == -1) {
//         printf("TCP state: %d, direction: %d, param: ", state, direction, permBits);
//         printf("fatal: TCP state transition wrong.\n");
//         exit(0);
//     }
//     return 0;
// }



int setTCPPacketReceiveCallback(TCPPacketReceiveCallback callback) {
    TCPPakcetCallback = callback;
}
int TCPCallback(const void* buf, int len, struct in_addr src, struct in_addr dst) {
    // printf("\n\nReceived packet!!!!!  %d\n\n", TCPchecksum(buf, len));
    if (TCPchecksum(buf, len) != 65535) return -1; // discard the wrong packet.


    uint16_t srcport = convertInt16PC2Net(*((uint16_t *)buf));
    uint16_t dstport = convertInt16PC2Net(*((uint16_t *)buf + 1));
    // swap(src, dst);
    // swap(srcport, dstport);

    uint32_t seqNumber = convertInt32PC2Net(*((uint32_t *)buf + 1));
    uint32_t ackNumber = convertInt32PC2Net(*((uint32_t *)buf + 2));

    uint16_t dataOffset = *((u_char *)buf + 12) >> 4;
    uint16_t permBits = *((u_char *)buf + 13) & ((1 << 6) - 1);
    uint16_t window = convertInt16PC2Net(*((uint16_t *)buf + 7));

    uint16_t urgentPointer = convertInt16PC2Net(*((uint16_t *)buf + 9));
    if (dataOffset != 5) return -1; // I don't know how to deal with options.
    
    struct tcpInfo *tcp = findTCPQuintuple(dst, src, dstport, srcport);
    if (tcp == NULL) return -1;

    // for (int i = 0; i < len; ++i)
    //     printf("%d ", *((u_char *)buf + i));
    // puts("\n\n");
    // u_char *buffer = (u_char *)malloc(len - 20);
    // if (buffer == NULL) return -1;
    // memcpy(buffer, buf, len - 20);

    // another thread.
    tcpReceiver(tcp, seqNumber, ackNumber, permBits, window, urgentPointer, buf + 20, len - 20); 
}



void tcpReceiver(struct tcpInfo *tcp, uint32_t seqNumber, uint32_t ackNumber, uint16_t permBits, uint16_t windowSize, uint16_t urgentPointer, const void *receiveBuffer, int len) {
    pthread_mutex_lock(&tcp -> lock);

    u_char buf[5];
    uint64_t tmp;
    int state;

    switch (tcp -> tcb.tcpState) {
    case LISTEN: // waiting for a connection request from any remote TCP and port.
        if (permBits & TCP_SYN) { // if it's synchronization.
            tcp -> tcb.RCV.IRS = seqNumber;
            tcp -> tcb.RCV.NXT = seqNumber + 1;
            tcp -> tcb.SND.WND = windowSize;
            // tcp -> tcb.SND.UNA = tcp -> tcb.SND.ISS;
            // tcp -> tcb.SND.NXT = tcp -> tcb.SND.ISS + 1;

            tcp -> lastClock = clock();
            __wrap_TCP2IPSender(tcp -> h, tcp -> tcb.SND.ISS, tcp -> tcb.RCV.NXT, TCP_SYN | TCP_ACK, tcp -> tcb.RCV.WND, 0, buf, 0);
            ++ tcp -> tcb.SND.NXT;
            tcp -> tcb.tcpState = TCP_SYN_RECV;
        }
        break;
    
    case TCP_SYN_SENT: // sent a connection request, waiting for ack
        if ((permBits & TCP_SYN) && (permBits & TCP_ACK)) { // rcv SYN,ACK
            if (ackNumber != tcp -> tcb.SND.UNA + 1) break; // acknumber wrong.
            tcp -> tcb.RCV.IRS = seqNumber;
            tcp -> tcb.RCV.NXT = seqNumber + 1;
            tcp -> tcb.SND.WND = windowSize;
            tcp -> tcb.SND.UNA = ackNumber;

            tcp -> lastClock = clock();
            // an old sequence number.
            __wrap_TCP2IPSender(tcp -> h, ackNumber - 1, tcp -> tcb.RCV.NXT, TCP_ACK, tcp -> tcb.RCV.WND, 0, buf, 0);
            tcp -> tcb.tcpState = TCP_ESTABLISHED;
        }
        else if (permBits & TCP_SYN) {
            tcp -> tcb.RCV.IRS = seqNumber;
            tcp -> tcb.RCV.NXT = seqNumber + 1;
            tcp -> tcb.SND.WND = windowSize;
            // it's Ack number is meaningless.

            tcp -> lastClock = clock();
            // an old sequence number.
            __wrap_TCP2IPSender(tcp -> h, tcp -> tcb.SND.NXT - 1, seqNumber + 1, TCP_ACK, tcp -> tcb.RCV.WND, 0, buf, 0);
            tcp -> tcb.tcpState = TCP_SYN_RECV;
        }
        break;
    
    case TCP_SYN_RECV: // received a connection request, sent ack, waiting for final ack in three-way handshake.
        if (permBits & TCP_ACK) {
            if (ackNumber != tcp -> tcb.SND.UNA + 1) break; // acknumber wrong.
            tcp -> tcb.SND.WND = windowSize;
            tcp -> tcb.SND.UNA = ackNumber;
            tcp -> tcb.SND.NXT = ackNumber;

            tcp -> tcb.tcpState = TCP_ESTABLISHED;
        }
        break;



    
    // 分手 有点难写, 等一会implement.
    case TCP_FIN_WAIT1: // our side has shutdown, waiting to complete transmission of remaining buffered data
        if (permBits & TCP_ACK) {
            if (ackNumber != tcp -> tcb.SND.NXT) break; // acknumber wrong.
            tcp -> tcb.tcpState = TCP_FIN_WAIT2;
        }
        break;

    case TCP_FIN_WAIT2: // all buffered data sent, waiting for remote to shutdown
        if (permBits & TCP_FIN) {
            tcp -> tcb.RCV.NXT = seqNumber + 1;
            tcp -> tcb.SND.WND = windowSize;

            tcp -> lastClock = clock();
            __wrap_TCP2IPSender(tcp -> h, tcp -> tcb.SND.NXT, tcp -> tcb.RCV.NXT, TCP_ACK, tcp -> tcb.RCV.WND, 0, buf, 0);
            tcp -> tcb.tcpState = TCP_TIME_WAIT;
        }
        break;

    case TCP_CLOSING: // both sides have shutdown but we still have data we have to finish sending
        if (permBits & TCP_ACK) {
            if (ackNumber != tcp -> tcb.SND.NXT) break; // acknumber wrong.
            tcp -> tcb.tcpState = TCP_TIME_WAIT;
        }
        break;

    case TCP_TIME_WAIT:// timeout to catch resent junk before entering closed, can only be entered from FIN_WAIT2
                        // or CLOSING.  Required because the other end may not have gotten our last ACK causing it
                        // to retransmit the data packet (which we ignore)
        // just drop the packet...
        break;
    case TCP_CLOSE_WAIT: // remote side has shutdown and is waiting for us to finish writing our data and to shutdown (we have to close() to move on to LAST_ACK)
        break;
    case TCP_LAST_ACK: // out side has shutdown after remote has shutdown.  There may still be data in our buffer that we have to finish sending
        if (permBits & TCP_ACK) {
            if (ackNumber != tcp -> tcb.SND.NXT) break; // acknumber wrong.
            tcp -> tcb.tcpState = TCP_CLOSE;
        }
        break;

    case TCP_CLOSE:
        // freeTCPConnect(tcp); must freed by sender.
        return;
        break;





    case TCP_ESTABLISHED: // connection established
        if (permBits & TCP_FIN) { // 怎么

        }
        else { // ordinary.
            // printf("Received data!\n");
            tcp -> tcb.SND.WND = windowSize; 
            if (seqNumber == tcp -> tcb.RCV.NXT) { // is what I want.
                tcp -> tcb.RCV.NXT += len;

                // for (int i = 0; i < len; ++ i) printf("%d ", *((u_char *)receiveBuffer + i));
                // puts("\n");
                state = ringBufferReceiveSegment(tcp -> rx, (u_char *)receiveBuffer, len);
                if (state == 0) { //receive, ack.
                    // an old sequence number.
                    printf("sending ACK.\n");
                    __wrap_TCP2IPSender(tcp -> h, tcp -> tcb.SND.NXT - 1, tcp -> tcb.RCV.NXT, TCP_ACK, tcp -> tcb.RCV.WND, 0, buf, 0);
                }
            }

            // check the ack for update.
            if ((permBits & TCP_ACK) && ackNumber != tcp -> tcb.SND.UNA) {
                printf("update UNA: %d %d\n", tcp -> tcb.SND.UNA, ackNumber);
                tcp -> tcb.SND.UNA = ackNumber;
            }
        }
        break;

    default:
        break;
    }

    pthread_mutex_unlock(&tcp -> lock);

}

/*
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |          Source Port          |       Destination Port        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        Sequence Number                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Acknowledgment Number                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Data |           |U|A|P|R|S|F|                               |
   | Offset| Reserved  |R|C|S|S|Y|I|            Window             |
   |       |           |G|K|H|T|N|N|                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           Checksum            |         Urgent Pointer        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Options                    |    Padding    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                             data                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
int __wrap_TCP2IPSender(struct tcpHeader *h, uint32_t seqNumber, uint32_t ackNumber, uint16_t permBits, uint16_t windowSize, uint16_t urgenPointer, const void *buf, int len) {
    if (len > MaxTCPSegment) return -1;
    u_char *buffer = (u_char *)malloc(len + 20); // no options.
    if (buffer == NULL) return -1;
    memset(buffer, 0, len + 20); // you need to clear the data!!!

    // printf("Send TCP packet!\n\n");

    *((uint16_t *)buffer) = convertInt16PC2Net(h -> srcport);
    *((uint16_t *)buffer + 1) = convertInt16PC2Net(h -> dstport);
    buffer += 4;
    *((uint32_t *)buffer) = convertInt32PC2Net(seqNumber);
    *((uint32_t *)buffer + 1) = convertInt32PC2Net(ackNumber);
    buffer += 8;
    *((uint16_t *)buffer) = convertInt16PC2Net(permBits | (5 << 12));
    *((uint16_t *)buffer + 1) = convertInt16PC2Net(windowSize);
    buffer += 4;
    *((uint16_t *)buffer + 1) = convertInt16PC2Net(urgenPointer);
    buffer -= 16;
    memcpy(buffer + 20, buf, len);

    // add checksum after others.
    // for (int i = 0; i < 10; ++i) *((uint16_t *)buffer + 8) += *((uint16_t *)buffer + i);
    *((uint16_t *)buffer + 8) = TCPchecksum(buffer, len + 20) ^ (65535);

    if (TCPchecksum(buffer, len + 20) != 65535) {
        printf("wrong!:  %d\n", len);
        while (1);
    }

    int state;
    // 从每个口都发一下...
    for (int i = 0; i < device_num; ++i) state = sendIPPacket(h -> src, h -> dst, 6, buffer, len + 20, 0, i);
    free(buffer);

    return state;
}

int TcpSlidingWindowSender(struct tcpInfo *tcp, int permBits) {
    if (tcp == NULL) return -1;
/*
        1           2         3          4 
    ----------|----------|----------|---------- 
           SND.UNA    SND.NXT    SND.UNA
                                +SND.WND
    1 - old sequence numbers which have been acknowledged
    2 - sequence numbers of unacknowledged data
    3 - sequence numbers allowed for new data transmission
    4 - future sequence numbers which are not yet allowed 
            Send Sequence Space
    int UNA; // send unacknowledged
    int NXT; // send next
    int WND; // send window
    int ISS; // initial send sequence number
*/

    // struct SendSequenceVariable SND = tcp -> tcb.SND;
    struct ringBuffer *que = tcp -> tx;
    int size = calcSize(que -> tail - que -> head);


    // printf("ring buffer data: %d %d %d %d\n", size, tcp -> tcb.SND.ISS, tcp -> tcb.SND.NXT, tcp -> tcb.SND.UNA);
    if (tcp -> tcb.SND.NXT == tcp -> tcb.SND.UNA + tcp -> tcb.SND.WND) return -1; // you can't exceed the window.
    if (size <= tcp -> tcb.SND.NXT - tcp -> tcb.SND.UNA) return -1; // no data to transfer.

    // can't exceed the (1) window (2) max segment (3) last dataa.
    int transmitSize = min(tcp -> tcb.SND.UNA + tcp -> tcb.SND.WND - tcp -> tcb.SND.NXT , min(MaxTCPSegment, size - (tcp -> tcb.SND.NXT - tcp -> tcb.SND.UNA)));
    u_char *buf = ringBufferSendSegment(que, transmitSize, tcp -> tcb.SND.NXT - tcp -> tcb.SND.UNA);

    
    int state = __wrap_TCP2IPSender(tcp -> h, tcp -> tcb.SND.NXT, tcp -> tcb.RCV.NXT, permBits, tcp -> tcb.RCV.WND, 0, buf, transmitSize);
    if (state == 0) {
        // sprintf("TCP has sent %d bytes data.\n", transmitSize);
        tcp -> tcb.SND.NXT += transmitSize;
    }
    return state;
}




// enum TCPstate{ 
//     TCP_TIME_WAIT,	  
//     TCP_CLOSE_WAIT,  
//     TCP_LAST_ACK,	  
// };*/
void* tcpSender(void *pt) {
    struct tcpInfo *tcp = (struct tcpInfo *)pt;
    int state = 0;

    u_char buf[5];
    uint64_t tmp;
    while (1) {
        pthread_mutex_lock(&tcp -> lock);

        switch (tcp -> tcb.tcpState) {
        case LISTEN: // waiting for a connection request from any remote TCP and port.
            // printf("tcp state: LISTEN\n");
            state = -1; // give up the lock.
            break;
        
        case TCP_SYN_SENT: // sent a connection request, waiting for ack
            // printf("tcp state: TCP_SYN_SENT\n");
            tmp = clock();
            // printf("time: %ld %ld\n", tcp ->lastClock, tmp);
            if (tmp - tcp -> lastClock < tcp -> RTT) { // didn't timeout.
                state = -1; // give up the lock.
            }
            else { // we need to send another syn message.
                // asd123www: 这里一开始syn的ack是多少？？？？
                tcp -> lastClock = tmp;
                state = __wrap_TCP2IPSender(tcp -> h, tcp -> tcb.SND.ISS, 0, TCP_SYN, tcp -> tcb.RCV.WND, 0, buf, 0);
                tcp -> tcb.SND.NXT = tcp -> tcb.SND.ISS + 1;
            }
            break;

        case TCP_SYN_RECV: // received a connection request, sent ack, waiting for final ack in three-way handshake.
            // printf("tcp state: TCP_SYN_RECV\n");
            if (tcp -> wantClose == 1) {
                // 发送FIN.
            }
            else {
                tmp = clock();
                if (tmp - tcp -> lastClock < tcp -> RTT) { // didn't timeout.
                    state = -1; // give up the lock.
                }
                else { // we need to send another syn message.
                    // asd123www: 这里一开始syn的ack是多少？？？？
                    tcp -> lastClock = tmp;
                    state = __wrap_TCP2IPSender(tcp -> h, tcp -> tcb.SND.ISS, tcp -> tcb.RCV.NXT, TCP_SYN | TCP_ACK, tcp -> tcb.RCV.WND, 0, buf, 0);
                }
            }
            break;




        // 分手 有点难写, 等一会implement.
        case TCP_FIN_WAIT1: // our side has shutdown, waiting to complete transmission of remaining buffered data
            tmp = clock();
            if (tmp - tcp -> lastClock < tcp -> RTT) {
                state = -1;
            }
            else { // retransmit the FIN.
                // 发送FIN.
                tcp -> lastClock = tmp;
                state = __wrap_TCP2IPSender(tcp -> h, tcp -> tcb.SND.NXT, tcp -> tcb.RCV.NXT, TCP_FIN, tcp -> tcb.RCV.WND, 0, buf, 0);
            }
            break;

        case TCP_FIN_WAIT2: // all buffered data sent, waiting for remote to shutdown
            state = -1;
            break;

        case TCP_CLOSING: // both sides have shutdown but we still have data we have to finish sending
            tmp = clock();
            if (tmp - tcp -> lastClock < tcp -> RTT) {
                state = -1;
            }
            else { // retransmit the FIN.
                // 发送FIN.
                tcp -> lastClock = tmp;
                state = __wrap_TCP2IPSender(tcp -> h, tcp -> tcb.SND.NXT, tcp -> tcb.RCV.NXT, TCP_FIN, tcp -> tcb.RCV.WND, 0, buf, 0);
            }
            break;

        case TCP_TIME_WAIT:// timeout to catch resent junk before entering closed, can only be entered from FIN_WAIT2
                           // or CLOSING.  Required because the other end may not have gotten our last ACK causing it
                           // to retransmit the data packet (which we ignore)
            usleep(2 * tcp -> RTT); // use RTT to estimate max...
            tcp -> tcb.tcpState = TCP_CLOSE;
            break;

        case TCP_CLOSE_WAIT: // remote side has shutdown and is waiting for us to finish writing our data and to shutdown (we have to close() to move on to LAST_ACK)
            tmp = clock();
            tcp -> lastClock = tmp;
            state = __wrap_TCP2IPSender(tcp -> h, tcp -> tcb.SND.NXT, tcp -> tcb.RCV.NXT, TCP_FIN, tcp -> tcb.RCV.WND, 0, buf, 0);
            tcp -> tcb.tcpState = TCP_LAST_ACK;
            break;


        case TCP_LAST_ACK: // out side has shutdown after remote has shutdown.  There may still be data in our buffer that we have to finish sending
            tmp = clock();
            if (tmp - tcp -> lastClock < tcp -> RTT) {
                state = -1;
            }
            else { // retransmit the FIN.
                // 发送FIN.
                tcp -> lastClock = tmp;
                state = __wrap_TCP2IPSender(tcp -> h, tcp -> tcb.SND.NXT, tcp -> tcb.RCV.NXT, TCP_FIN, tcp -> tcb.RCV.WND, 0, buf, 0);
            }
            break;


        case TCP_CLOSE:
            freeTCPConnect(tcp);
            return NULL;
            break;



        

        case TCP_ESTABLISHED: // connection established
            // printf("tcp state: TCP_ESTABLISHED\n");
            tmp = clock();
            if (tcp -> wantClose == 1) {
                // 发送FIN.
                tcp -> lastClock = tmp;
                state = __wrap_TCP2IPSender(tcp -> h, tcp -> tcb.SND.NXT, tcp -> tcb.RCV.NXT, TCP_FIN, tcp -> tcb.RCV.WND, 0, buf, 0);
                tcp -> tcb.tcpState = TCP_FIN_WAIT1;
            }
            else {
                if (tmp - tcp -> lastClock > tcp -> RTT) { // timeout
                    tcp -> tcb.SND.NXT = tcp -> tcb.SND.UNA; // retransmit the first byte.
                }
                tcp -> lastClock = tmp; // update the timer.
                state = TcpSlidingWindowSender(tcp, TCP_ACK);
            }
            break;
        
        default:
            break;
        }
        pthread_mutex_unlock(&tcp -> lock);
        
        if (state != 0) usleep(5e5); // if nothing the sender did, then sleep for specific time hoping for state change.
    }
}