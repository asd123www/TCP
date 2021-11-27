
#include "tcp.h"

int convertInt16PC2Net(uint32_t x) {
    return ((x & 255) << 8) | (x >> 8 & 255);
}

int convertInt32PC2Net(uint32_t x) {
    return (convertInt16PC2Net(x & 65535) << 16) | convertInt16PC2Net(x >> 16 & 65535);
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


/* the wrapper function, just send, no reliability concern.
 * return 0 if succeeded, else wrong.
 */
int tcpWrapperSender(struct tcpHeader *h, uint32_t seqNumber, uint32_t ackNumber, uint16_t permBits, uint16_t windowSize, uint16_t urgenPointer, const void *buf, int len) {
    if (len > MaxTcpSegment) return -1;
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
    // add checksum after others.
    for (int i = 0; i < 10; ++i) *((uint16_t *)buffer + 8) += *((uint16_t *)buffer + i);
    *((uint16_t *)buffer + 8) ^= 65535;
    memcpy(buffer + 20, buf, len);

    int state = sendIPPacket(h -> src, h -> dst, 6, buffer, len + 20, 0, 0);
    free(buffer);

    return state;
}


/*
    Basic Data Transfer
    Reliability
    Flow Control
    Multiplexing
    Connections
    Precedence and Security
*/