#include <stdio.h>
#include <pthread.h>

#include "checkpoints/ip.h"
#include <unistd.h>


void printOutRouterTable(int n) {
    // printf("On device ");
    for(int i = 0; i < n; ++i) {
        sync_printf("number %d device's MAC address: ", i+1);
        for(int j = 0; j < 5; ++j) sync_printf("%02x.", subnet_list[i] -> Mac[j]);
        sync_printf("%02x\n", subnet_list[i] -> Mac[5]);
    }
    for(int i = 1; i <= n; ++i) sync_printf("%d ", dist[0][i]);
    sync_printf("\n");
}

/*
 * send router algo info periodically.
 * WARNING: the sleep is not accurate for the non-guaranteed kernel scheduling
 */
void* packetSenderThreadRouterFunction() {
    memset(edge, 0x3f, sizeof(edge));

    //sendFrame(const void* buf, int len, int ethtype, const void* destmac, int id);
    char buf[100];
    char macAddress[10];
    for(int i=0; i<6; ++i) macAddress[i]=0xff;
    struct in_addr src;
    struct in_addr dest;

    while (1) {
        sync_printf("new iteration:\n");
        // int sendIPPacket(const struct in_addr src, const struct in_addr dest, int proto, const void *buf, int len) {
        // src.s_addr = (172<<24) | (16<<16) | (140<<8) | 2;

        for (int device = 0; device < device_num; ++device) {
            src.s_addr = device_list[device] -> ip;
            dest.s_addr = (255<<24) | (255<<16) | (255<<8) | 255;

            for(int i=0;i<10;++i) buf[i] = 66; // fixed string for recognition of router messages.
            for(int i=10;i<16;++i) buf[i] = (device_list[device] -> mac >> 8 * (15-i)) & 255;

            // sendFrame(buf, 16, 0x0800, macAddress, device);
            sendIPPacket(src, dest, 6, buf, 16, 1, device); // need broadcast.
        }
        sync_printf("Finished broadcasting null message.\n");
        sleep(5);
        // memset(edge, 0x3f, sizeof(edge)); // 清空连接情况.


        for (int device = 0; device < device_num; ++device) {
            src.s_addr = device_list[device] -> ip;
            dest.s_addr = (255<<24) | (255<<16) | (255<<8) | 255;

            for(int i=0;i<10;++i) buf[i] = 66; // fixed string for recognition of router messages.
            for(int i=10;i<16;++i) buf[i] = (device_list[device] -> mac >> 8 * (15-i)) & 255;
            int len = 16;
            for(int i = 0; i < subnet_num; ++i) if(edge[0][i+1] == 1) {
                for(int j = 0; j < 6; ++j) buf[len + j] = subnet_list[i] -> Mac[j];
                len += 6;
            }
            sendIPPacket(src, dest, 6, buf, len, 1, device); // need broadcast.
        }
        sync_printf("Finished broadcasting directly connected message.\n");
        sleep(5);
        
        sync_printf("subnet number: %d\n", subnet_num);
        Dijkstra(subnet_num + 1);
        printOutRouterTable(subnet_num);
        setEdgeEntry(0x3f, -1, -1);
        sync_printf("\n\n\n\n");
    }
}

/*
 * enter the receive, keeps tracking the packets received and update the router algo info.
 */
void* packetReceiverThreadRouterFunction(void *p) {
    int device = (int)p;
    setFrameReceiveCallback(frameCallbackExample, device); // 监听device收包.
}


int main(int argc, char *argv[]) {
    test();
    setIPPacketReceiveCallback(ipCallbackExample);



    pthread_t packet_sender;
    pthread_t packet_receiver[5];

    sync_printf("device number: %d\n", device_num);
    for(int i = 0; i < device_num; ++i) {
        if (device_list[i] == NULL) continue;
        pthread_create(packet_receiver + i, NULL, packetReceiverThreadRouterFunction, (void *)(i));
    }
    pthread_create(&packet_sender, NULL, packetSenderThreadRouterFunction, NULL);

    pthread_exit(NULL);
    return 0;
}
/*
gcc test.c -o a -lpcap -lpthread
*/