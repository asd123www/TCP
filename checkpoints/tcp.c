
#include "tcp.h"



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
    tcp -> RTT = CLOCKS_PER_SEC * 1.5; // 1.5s

    tcp -> h = (struct tcpHeader *)malloc(sizeof(struct tcpHeader));
    *(tcp -> h) = hd;

    tcp -> rx = initRingBuffer();
    tcp -> tx = initRingBuffer();

    // asd123www: maybe listen or sendsyn? must be these two types.
    tcp -> tcb.tcpState = state; // the caller define the initial type.
    tcp -> tcb.SND.WND = INITIAL_WINDOW_SIZE; // initiate a window size.
    tcp -> tcb.SND.ISS = tcp -> tcb.SND.UNA = tcp -> tcb.SND.NXT = clock(); // maybe 64-bit? just module 2^32.
    // asd123www: I don't know the meaning of UP or WL1 or WL2.


    // RCV.IRS and RCV.NXT don't know before handshake.
    tcp -> tcb.RCV.WND = INITIAL_WINDOW_SIZE; // It's decided by me, so just be a constant.

    return tcp;
}

void freeTCPConnect(struct tcpInfo *tcp) {
    if (tcp == NULL) return;

    free(tcp -> h);
    freeRingBuffer(tcp -> rx);
    freeRingBuffer(tcp -> tx);
    free(tcp);
    return;
}


int convertInt16PC2Net(uint32_t x) {
    return ((x & 255) << 8) | (x >> 8 & 255);
}

int convertInt32PC2Net(uint32_t x) {
    return (convertInt16PC2Net(x & 65535) << 16) | convertInt16PC2Net(x >> 16 & 65535);
}



int __wrap_pushTCPData(struct tcpInfo *fd, u_char *buf, int len) {
    if (fd == NULL) return -1;
    int state = ringBufferReceiveSegment(fd -> tx, buf, len);
    return state;
}

int pushTCPData(struct tcpInfo *fd, u_char *buf, int len) {
    pthread_mutex_lock(&fd -> lock);
    int state = __wrap_pushTCPData(fd, buf, len);
    pthread_mutex_unlock(&fd -> lock);
    return state;
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
    return state;
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
int TCPCallback(const void* buf, int len) {
    
}


uint16_t TCPchecksum(const void *buf, int len) {
    uint16_t ans = 0;
    uint16_t *begin = (uint16_t *)buf;
    for (int i = 0; i < len/2; ++i) ans += *(begin + i);
    if (len & 1) ans += (*((uint8_t *)buf + len - 1)) << 8;
    return ans;
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

    int state = sendIPPacket(h -> src, h -> dst, 6, buffer, len + 20, 0, 0);
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

    struct SendSequenceVariable SND = tcp -> tcb.SND;
    struct ringBuffer *que = tcp -> tx;
    int size = calcSize(que -> tail - que -> head);

    if (SND.NXT == SND.UNA + SND.WND) return -1; // you can't exceed the window.
    if (size <= SND.NXT - SND.UNA) return -1; // no data to transfer.

    // can't exceed the (1) window (2) max segment (3) last dataa.
    int transmitSize = min(SND.UNA + SND.WND - SND.NXT , min(MaxTCPSegment, size - (SND.NXT - SND.UNA)));
    u_char *buf = ringBufferSendSegment(que, transmitSize, SND.NXT - SND.UNA);

    
    int state = __wrap_TCP2IPSender(tcp -> h, SND.NXT, tcp -> tcb.RCV.NXT, permBits, tcp -> tcb.RCV.WND, 0, buf, transmitSize);
    if (state == 0) SND.NXT += transmitSize;
    return state; 
}




// enum TCPstate{ 
//     TCP_TIME_WAIT,	  
//     TCP_CLOSE_WAIT,  
//     TCP_LAST_ACK,	  
// };*/
void tcpSender(struct tcpInfo *tcp) {
    int state = 0;
    
    u_char buf[5];
    uint64_t tmp;
    while (1) {
        pthread_mutex_lock(&tcp -> lock);

        switch (tcp -> tcb.tcpState) {
        case LISTEN: // waiting for a connection request from any remote TCP and port.
            state = -1; // give up the lock.
            break;
        
        case TCP_SYN_SENT: // sent a connection request, waiting for ack
            tmp = clock();
            if (tmp - tcp -> lastClock < tcp -> RTT) { // didn't timeout.
                state = -1; // give up the lock.
            }
            else { // we need to send another syn message.
                // asd123www: 这里一开始syn的ack是多少？？？？
                tcp -> lastClock = clock();
                state = __wrap_TCP2IPSender(tcp -> h, tcp -> tcb.SND.ISS, 0, TCP_SYN, tcp -> tcb.RCV.WND, 0, buf, 0);
            }
            break;

        case TCP_SYN_RECV: // received a connection request, sent ack, waiting for final ack in three-way handshake.
            state = -1;
            break;

        // 分手 有点难写, 等一会implement.
        case TCP_FIN_WAIT1: // our side has shutdown, waiting to complete transmission of remaining buffered data
            break;
        case TCP_FIN_WAIT2: // all buffered data sent, waiting for remote to shutdown
            break;
        case TCP_CLOSING: // both sides have shutdown but we still have data we have to finish sending
            break;
        case TCP_TIME_WAIT:// timeout to catch resent junk before entering closed, can only be entered from FIN_WAIT2
                           // or CLOSING.  Required because the other end may not have gotten our last ACK causing it
                           // to retransmit the data packet (which we ignore)
            break;
        case TCP_CLOSE_WAIT: // remote side has shutdown and is waiting for us to finish writing our data and to shutdown (we have to close() to move on to LAST_ACK)
            break;
        case TCP_LAST_ACK: // out side has shutdown after remote has shutdown.  There may still be data in our buffer that we have to finish sending
            break;
        

        case TCP_CLOSE:
            freeTCPConnect(tcp);
            return;
            break;
        

        case TCP_ESTABLISHED: // connection established
            tmp = clock();
            if (tmp - tcp -> lastClock > tcp -> RTT) { // timeout
                tcp -> tcb.SND.NXT = tcp -> tcb.SND.UNA; // retransmit the first byte.
            }
            tcp -> lastClock = tmp; // update the timer.
            state = TcpSlidingWindowSender(tcp, 0);
            break;
        
        default:
            break;
        }
        pthread_mutex_unlock(&tcp -> lock);

        if (state != 0) usleep(1e5); // if nothing the sender did, then sleep for specific time hoping for state change.
    }
}