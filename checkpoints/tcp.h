#ifndef TCP_H
#define TCP_H

#include <time.h> //CLOCKS_PER_SEC
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

#define MaxTCPNumber 100
#define MaxTCPSegment 5

#define mySocketNumberOffset 123436

#define INITIAL_WINDOW_SIZE 100

#define TCP_URG 32
#define TCP_ACK 16
#define TCP_PSH 8
#define TCP_RST 4
#define TCP_SYN 2
#define TCP_FIN 1

#define ACTION_RECV 0
#define ACTION_SEND 1

// TCP states
enum TCPstate{
    LISTEN = 0, 
    TCP_SYN_SENT, // sent a connection request, waiting for ack
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
    uint32_t UNA; // send unacknowledged
    uint32_t NXT; // send next
    uint32_t WND; // send window
    uint32_t UP;  // send urgent pointer
    uint32_t WL1; // segment sequence number used for last window update
    uint32_t WL2; // segment acknowledgment number used for last window update
    uint32_t ISS; // initial send sequence number
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
    uint32_t NXT; // receive next
    uint32_t WND; // receive window
    uint32_t UP;  // receive urgent pointer
    uint32_t IRS; // initial receive sequence number
};

struct tcpHeader {
    uint16_t srcport;
    uint16_t dstport;
    struct in_addr src;
    struct in_addr dst;
};

struct TransmissionControlBlock {
    int tcpState;
    struct SendSequenceVariable SND;
    struct ReceiveSequenceVariable RCV;
};


// quintuple(五元组) identify the communication channel.

struct tcpInfo {
    pthread_mutex_t lock; // syn for send/receive.


    int wantClose;

    uint64_t lastClock;
    uint64_t RTT;

    struct tcpHeader *h;

    struct ringBuffer *rx;
    struct ringBuffer *tx;

    struct TransmissionControlBlock tcb;
};

struct tcpInfo *tcps[MaxTCPNumber];



struct socketPair {
    int type;
    void *addr;
};

struct socketPair mapSocket[2 * MaxTCPNumber];

/* @brief: alloc smallest free identifier.
 *
 * @param type: tcpInfo or Socket.
 * @param addr: pointer to the struct.
 * return 0 if succeed, otherwise -1.
 */ 
int allocSocketDescriptor(int type, void *addr);

/* @brief: transfer the socket number to real index.
 *
 * @param type: tcpInfo or Socket.
 * @param addr: pointer to the struct.
 * return the >= 0 real index if succeed, otherwise -1.
 */ 
int maintainSocketDescriptor(int fd);




int convertInt16PC2Net(uint32_t x);
int convertInt32PC2Net(uint32_t x);

/* @brief: initialize a tcp connect.
 * 
 * @param state: initial tcp state.
 */
struct tcpInfo *initiateTCPConnect(struct tcpHeader hd, int state);

/* @brief: free the tcp connect.
 * 
 */
void freeTCPConnect(struct tcpInfo *tcp);

/* @brief: find the tcp connection based on the 5-tuple.
 *         the protocol is specific.
 */
struct tcpInfo *findTCPQuintuple(struct in_addr src, struct in_addr dst, uint16_t srcport, uint16_t dstport);
int deleteTCPQuintuple(struct tcpInfo *tcp);

// /* @brief: initialize the transfer matrix.
//  * 
//  */
// void initialMatStateAction();

// /* @brief: change the TCP state after specific action.
//  * 
//  * return 0 if succeed.
//  * otherwise -1.
//  */
// int TCPStateTransferWithAction(struct TransmissionControlBlock *tcb, int direction, int param, int isTimeOut);


int checkSocketNumberValid(int socketNumber);


uint16_t TCPchecksum(const void *buf, int len);

/* the wrapper function, just send, no reliability concern.
 * return 0 if succeeded, else wrong.
 * Current Segment Variables
    SEG.SEQ - segment sequence number
    SEG.ACK - segment acknowledgment number
    SEG.LEN - segment length
    SEG.WND - segment window
    SEG.UP  - segment urgent pointer
    SEG.PRC - segment precedence value
 */
int __wrap_TCP2IPSender(struct tcpHeader *h, uint32_t seqNumber, uint32_t ackNumber, uint16_t permBits, uint16_t windowSize, uint16_t urgenPointer, const void *buf, int len);


/* @brief: send a segment in TCP's ring buffer. Based on tcb.
 *
 * @param tcp: the tcp connection.
 * @param permBits: the 6 state bits.
 */
int TcpSlidingWindowSender(struct tcpInfo *tcp, int permBits);

/* @brief: the thread for TCP sender. Never return until closed.
 *
* @param pt: pointer to tcp
 */
void* tcpSender(void *pt);

/* @brief: the function for TCP receiver. Return after executing the packet.
 *
 */
void tcpReceiver(struct tcpInfo *tcp, uint32_t seqNumber, uint32_t ackNumber, uint16_t permBits, uint16_t windowSize, uint16_t urgentPointer, const void *receiveBuffer, int len); 



/* @brief: push len bytes data to tcp connnect fd's buffer.
 * 
 * @param fd: the tcp connect.
 * @param buf: source buffer.
 * @param len: length of data.
 * 
 * return 0 if succeed.
 * else return -1;
 */
int pushTCPData(struct tcpInfo *fd, const u_char *buf, int len);

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




/*
 * @brief Register a callback function to be called each time an IP packet was received.
 * 
 * @param callback The callback function.
 * @return 0 on success, -1 on error.
 * @see IPPacketReceiveCallback
 */
int setTCPPacketReceiveCallback(TCPPacketReceiveCallback callback);
int TCPCallback(const void* buf, int len, struct in_addr src, struct in_addr dst);

#endif // SOCKET_H