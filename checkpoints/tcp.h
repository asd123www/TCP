#ifndef TCP_H
#define TCP_H

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <string.h>

#include "ip.h"

typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;

#define MaxConnectNumber 10
#define MaxTcpSegment 500

/* Description of States:
 *
 *	TCP_SYN_SENT		sent a connection request, waiting for ack
 *
 *	TCP_SYN_RECV		received a connection request, sent ack,
 *				waiting for final ack in three-way handshake.
 *
 *	TCP_ESTABLISHED		connection established
 *
 *	TCP_FIN_WAIT1		our side has shutdown, waiting to complete
 *				transmission of remaining buffered data
 *
 *	TCP_FIN_WAIT2		all buffered data sent, waiting for remote
 *				to shutdown
 *
 *	TCP_CLOSING		both sides have shutdown but we still have
 *				data we have to finish sending
 *
 *	TCP_TIME_WAIT		timeout to catch resent junk before entering
 *				closed, can only be entered from FIN_WAIT2
 *				or CLOSING.  Required because the other end
 *				may not have gotten our last ACK causing it
 *				to retransmit the data packet (which we ignore)
 *
 *	TCP_CLOSE_WAIT		remote side has shutdown and is waiting for
 *				us to finish writing our data and to shutdown
 *				(we have to close() to move on to LAST_ACK)
 *
 *	TCP_LAST_ACK		out side has shutdown after remote has
 *				shutdown.  There may still be data in our
 *				buffer that we have to finish sending
 *
 *	TCP_CLOSE		socket is finished
 */


struct tcpHeader {
    int srcport;
    int dstport;
    struct in_addr src;
    struct in_addr dst;
};


struct tcpInfo {
    struct tcpHeader *h;

    pthread_mutex_t lock; // syn for send/receive.

    struct ringBuffer *rx;
    struct ringBuffer *tx;
};

struct tcpInfo *tcps[MaxConnectNumber];



int convertInt16PC2Net(uint32_t x);
int convertInt32PC2Net(uint32_t x);

int findLowestSocketNumber();

int checkSocketNumberValid(int socketNumber);



int tcpWrapperSender(struct tcpHeader *h, uint32_t seqNumber, uint32_t ackNumber, uint16_t permBits, uint16_t windowSize, uint16_t urgenPointer, const void *buf, int len);



#endif // SOCKET_H