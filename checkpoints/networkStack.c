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

int main(int argc, char *argv[]) {
    pthread_mutex_init(&printfMutex, NULL);
    pthread_mutex_init(&setRoutingTableLock, NULL);
    pthread_mutex_init(&edgeEntryLock, NULL);

    addDevices();
    setIPPacketReceiveCallback(ipCallbackExample);
    setTCPPacketReceiveCallback(TCPCallback);


    pthread_t packet_sender;
    pthread_t packet_receiver[10];

    sync_printf("device number: %d\n", device_num);
    for(int i = 0; i < device_num; ++i) {
        if (device_list[i] == NULL) continue;
        pthread_create(packet_receiver + i, NULL, packetReceiverThreadRouterFunction, (void *)(i));
    }

    pthread_exit(NULL);
    return 0;
}