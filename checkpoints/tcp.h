#ifndef TCP_H
#define TCP_H

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <string.h>

#include "ip.h"
#include "ringbuffer.h"

typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;

#define MaxTCPNumber 10
#define MaxTCPSegment 500

// TCP states
enum TCPstate{
    TCP_SYN_SENT = 0, // sent a connection request, waiting for ack
    TCP_SYN_RECV,     // received a connection request, sent ack, waiting for final ack in three-way handshake.
    TCP_ESTABLISHED,  // connection established
    TCP_FIN_WAIT1,    // our side has shutdown, waiting to complete transmission of remaining buffered data
    TCP_FIN_WAIT2,	  // all buffered data sent, waiting for remote to shutdown
    TCP_CLOSING,	  // both sides have shutdown but we still have data we have to finish sending
    TCP_TIME_WAIT,	  // timeout to catch resent junk before entering closed, can only be entered from FIN_WAIT2
                      // or CLOSING.  Required because the other end may not have gotten our last ACK causing it
                      // to retransmit the data packet (which we ignore)
    TCP_CLOSE_WAIT,   // remote side has shutdown and is waiting for us to finish writing our data and to shutdown (we have to close() to move on to LAST_ACK)
    TCP_LAST_ACK,	  // out side has shutdown after remote has shutdown.  There may still be data in our buffer that we have to finish sending
    TCP_CLOSE		  // socket is finished
};

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
*/
struct SendSequenceVariable {
    int UNA; // send unacknowledged
    int NXT; // send next
    int WND; // send window
    int UP;  // send urgent pointer
    int WL1; // segment sequence number used for last window update
    int WL2; // segment acknowledgment number used for last window update
    int ISS; // initial send sequence number
};


/*
        1          2          3 
    ----------|----------|----------
           RCV.NXT    RCV.NXT
                     +RCV.WND
    1 - old sequence numbers which have been acknowledged
    2 - sequence numbers allowed for new reception
    3 - future sequence numbers which are not yet allowed 
        Receive Sequence Space
*/
struct ReceiveSequenceVariable {
    int NXT; // receive next
    int WND; // receive window
    int UP;  // receive urgent pointer
    int IRS; // initial receive sequence number
};

struct tcpHeader {
    int srcport;
    int dstport;
    struct in_addr src;
    struct in_addr dst;
};

struct TransmissionControlBlock {
    int tcpState;
    struct SendSequenceVariable SND;
    struct ReceiveSequenceVariable RCV;
};

struct tcpInfo {
    pthread_mutex_t lock; // syn for send/receive.

    struct tcpHeader *h;

    struct ringBuffer *rx;
    struct ringBuffer *tx;

    struct TransmissionControlBlock tcb;
};

struct tcpInfo *tcps[MaxTCPNumber];







int convertInt16PC2Net(uint32_t x);
int convertInt32PC2Net(uint32_t x);

int findLowestSocketNumber();

int checkSocketNumberValid(int socketNumber);


uint16_t TCPchecksum(const void *buf, int len);

/* the wrapper function, just send, no reliability concern.
 * return 0 if succeeded, else wrong.
 */
int __wrap_TCP2IPSender(struct tcpHeader *h, uint32_t seqNumber, uint32_t ackNumber, uint16_t permBits, uint16_t windowSize, uint16_t urgenPointer, const void *buf, int len);



/* @brief: read len bytes data from tcp connnect fd to buffer.
 * 
 * @param fd: the tcp connect.
 * @param buf: destination buffer.
 * @param len: length of data.
 * 
 * return 0 if succeed.
 * else return -1;
 */ 
int fetchTCPData(struct tcpInfo *fd, u_char *buf, int len);

#endif // SOCKET_H