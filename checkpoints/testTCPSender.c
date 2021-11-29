#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#include "tcp.h"

/*
 * enter the receive, keeps tracking the packets received and update the router algo info.
 */
void* packetReceiverThreadRouterFunction(void *p) {
    int device = (int)p;
    setFrameReceiveCallback(frameCallbackExample, device); // 监听device收包.
}

void* testSendTcpPakcet() {
    struct tcpHeader hd;
    #define ipAddress(a,b,c,d) ((d<<24) | (c<<16) | (b<<8) | a)
    hd.src.s_addr = ipAddress(10, 100, 1, 1); // 10.100.1.1
    hd.dst.s_addr = ipAddress(10, 100, 1, 2);
    hd.dstport = 4321;
    hd.srcport = 1234;

    struct tcpInfo* tcp = initiateTCPConnect(hd, TCP_SYN_SENT);

    char buf[1000];

    for(int i = 0; i < 999; ++i) buf [i] = i;

    int state = pushTCPData(tcp, buf, 1000);
    int size = calcSize(tcp -> tx -> tail - tcp -> tx -> head);

    printf("%d\n", size);
    printf("push data into TCP: %d\n", state);

    tcpSender((void *)tcp); // never return.
}



int main(int argc, char *argv[]) {
    pthread_mutex_init(&printfMutex, NULL);
    pthread_mutex_init(&setRoutingTableLock, NULL);
    pthread_mutex_init(&edgeEntryLock, NULL);


    addDevices();
    setIPPacketReceiveCallback(ipCallbackExample);
    setTCPPacketReceiveCallback(TCPCallback);


    pthread_t tcp_sender;
    pthread_t packet_receiver[10];

    sync_printf("device number: %d\n", device_num);
    for(int i = 0; i < device_num; ++i) {
        if (device_list[i] == NULL) continue;
        pthread_create(packet_receiver + i, NULL, packetReceiverThreadRouterFunction, (void *)(i));
    }

    pthread_create(&tcp_sender, NULL, testSendTcpPakcet, NULL);
    // pthread_create(&packet_sender, NULL, packetSenderThreadRouterFunction, NULL);

    pthread_exit(NULL);
    return 0;
}