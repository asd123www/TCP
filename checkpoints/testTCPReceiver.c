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

void* testReceiveTcpPakcet() {
    struct tcpHeader hd;
    #define ipAddress(a,b,c,d) ((d<<24) | (c<<16) | (b<<8) | a)
    hd.src.s_addr = ipAddress(10, 100, 1, 2); // 10.100.1.1
    hd.dst.s_addr = ipAddress(10, 100, 1, 1); 
    hd.srcport = 4321;
    hd.dstport = 1234;

    struct tcpInfo* tcp = initiateTCPConnect(hd, LISTEN);

    char buf[1000];

    // sender for sending ack...
    pthread_t tcp_sender;
    pthread_create(&tcp_sender, NULL, tcpSender, tcp);

    int size = 0;
    while (1) {
        int state = fetchTCPData(tcp, buf, 5);
        printf("fetch TCP data: %d\n", state);
        if (state != -1) {
            size += 5;
            printf("Successfully received packet!  %d %d\n", size, calcSize(tcp -> rx ->tail -tcp -> rx ->head ));
            for(int i = 0; i < 5; ++i) printf("%d ", buf[i]);
            puts("");
        }
        else sleep(1);

        if (size > 200) {
        //     tcp -> wantClose = 1;
        }
    }
}



int main(int argc, char *argv[]) {
    pthread_mutex_init(&printfMutex, NULL);
    pthread_mutex_init(&setRoutingTableLock, NULL);
    pthread_mutex_init(&edgeEntryLock, NULL);


    addDevices();
    setIPPacketReceiveCallback(ipCallbackExample);
    setTCPPacketReceiveCallback(TCPCallback);


    pthread_t tcp_receiver;
    pthread_t packet_receiver[10];

    sync_printf("device number: %d\n", device_num);
    for(int i = 0; i < device_num; ++i) {
        if (device_list[i] == NULL) continue;
        pthread_create(packet_receiver + i, NULL, packetReceiverThreadRouterFunction, (void *)(i));
    }
    pthread_create(&tcp_receiver, NULL, testReceiveTcpPakcet, NULL);

    pthread_exit(NULL);
    return 0;
}