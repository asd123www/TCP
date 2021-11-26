/**
 * @file device.h
 * @brief Library supporting network device management. 
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pcap/pcap.h>
#include <pthread.h>

#include "device.h"

pthread_mutex_t printfMutex;

uint64_t uchar2int64Mac(u_char *buf) {
    uint64_t ans = 0;
    for(int i = 0; i < 6; ++i) ans = ans << 8 | (((u_char *)buf)[i]);
    return ans;
}

int findDevice(const char *device) {
    for (int i = 0; i < MAX_DEVICE_NUMBER; ++i) {
        struct net_device *iter = device_list[i];
        if (iter == NULL || strcmp(iter -> name, device) != 0) continue;
        return i;
    }
    return -1;
}

int addDevice(const char* device) {

    static pcap_if_t *head;
    // struct pcap_addr t;
    if (head == NULL) {
        int state = pcap_findalldevs(&head, errbuf);
        if (state == -1) return -1;
    }

    for (pcap_if_t *iter = head; iter != NULL; iter = iter->next) {
        // printf("name: %s\n", iter->name);
        if (strcmp(iter->name, device) != 0) continue;

        struct net_device *ptr = (struct net_device *)malloc(sizeof(struct net_device));
        if (ptr == NULL) {
            // fprintf(stderr,"\nUnable to malloc the ptr.\n");
            return -1;
        }
        ptr -> name = (char *)malloc(sizeof(device));
        if (ptr -> name == NULL) {
            // fprintf(stderr,"\nUnable to malloc the name.\n");
            return -1;
        }
        strcpy(ptr -> name, device);

        for (pcap_addr_t *i = iter->addresses; i != NULL; i = i->next) { // address family for us.
            if (i->addr == NULL) continue;
            if (i->addr->sa_family == AF_INET) { // ipv4
                uint32_t a_addr = ((struct sockaddr_in *)i->addr)->sin_addr.s_addr;
                ptr -> ip = a_addr;
                // printf("address: %d.%d.%d.%d\n", a_addr & (255), (a_addr>>8) & 255, (a_addr>>16) & 255, (a_addr>>24));
            }
            else if(i->addr->sa_family == AF_INET6) { // ipv6
            
            }
            else { //if(i->addr->sa_family == ){
                ptr -> mac = 0;
                for (int j=10; j<16; ++j) { // seems max to 14...
                    ptr -> mac <<= 8;
                    ptr -> mac |= i->addr->sa_data[j] & 255;
                }
            }
        }

        device_list[device_num++] = ptr;
        return 1;
    }
    return -1;
}


int sync_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);

    pthread_mutex_lock(&printfMutex);
    vprintf(format, args);
    fflush(stdout);
    pthread_mutex_unlock(&printfMutex);

    va_end(args);
}

void test() {
    char a[20];
    char b[20];
    a[0] = 'e';
    a[1] = 'n';
    a[2] = 's';
    a[3] = '3';
    a[4] = '3';
    a[5] = '\0';
    sync_printf("state: %d\n\n", addDevice(a));

    b[0] = 'v';
    b[1] = 'i';
    b[2] = 'r';
    b[3] = 'b';
    b[4] = 'r';
    b[5] = '0';
    b[6] = '\0';
    sync_printf("state: %d\n\n", addDevice(b));


    b[0] = 'v';
    b[1] = 'e';
    b[2] = 't';
    b[3] = 'h';
    b[4] = '1';
    b[5] = '-';
    b[6] = '2';
    b[7] = '\0';
    sync_printf("state: %d\n\n", addDevice(b));


    b[0] = 'v';
    b[1] = 'e';
    b[2] = 't';
    b[3] = 'h';
    b[4] = '2';
    b[5] = '-';
    b[6] = '1';
    b[7] = '\0';
    sync_printf("state: %d\n\n", addDevice(b));

    b[0] = 'v';
    b[1] = 'e';
    b[2] = 't';
    b[3] = 'h';
    b[4] = '2';
    b[5] = '-';
    b[6] = '3';
    b[7] = '\0';
    sync_printf("state: %d\n\n", addDevice(b));

    b[0] = 'v';
    b[1] = 'e';
    b[2] = 't';
    b[3] = 'h';
    b[4] = '3';
    b[5] = '-';
    b[6] = '0';
    b[7] = '\0';
    sync_printf("state: %d\n\n", addDevice(b));

    b[0] = 'v';
    b[1] = 'e';
    b[2] = 't';
    b[3] = 'h';
    b[4] = '3';
    b[5] = '-';
    b[6] = '2';
    b[7] = '\0';
    sync_printf("state: %d\n\n", addDevice(b));

    b[0] = 'v';
    b[1] = 'e';
    b[2] = 't';
    b[3] = 'h';
    b[4] = '3';
    b[5] = '-';
    b[6] = '4';
    b[7] = '\0';
    sync_printf("state: %d\n\n", addDevice(b));

     b[0] = 'v';
    b[1] = 'e';
    b[2] = 't';
    b[3] = 'h';
    b[4] = '4';
    b[5] = '-';
    b[6] = '3';
    b[7] = '\0';
    sync_printf("state: %d\n\n", addDevice(b));


    b[0] = 'v';
    b[1] = 'e';
    b[2] = 't';
    b[3] = 'h';
    b[4] = '2';
    b[5] = '-';
    b[6] = '5';
    b[7] = '\0';
    sync_printf("state: %d\n\n", addDevice(b));

    b[0] = 'v';
    b[1] = 'e';
    b[2] = 't';
    b[3] = 'h';
    b[4] = '5';
    b[5] = '-';
    b[6] = '2';
    b[7] = '\0';
    sync_printf("state: %d\n\n", addDevice(b));

    b[0] = 'v';
    b[1] = 'e';
    b[2] = 't';
    b[3] = 'h';
    b[4] = '5';
    b[5] = '-';
    b[6] = '6';
    b[7] = '\0';
    sync_printf("state: %d\n\n", addDevice(b));

    b[0] = 'v';
    b[1] = 'e';
    b[2] = 't';
    b[3] = 'h';
    b[4] = '6';
    b[5] = '-';
    b[6] = '5';
    b[7] = '\0';
    sync_printf("state: %d\n\n", addDevice(b));

    b[0] = 'v';
    b[1] = 'e';
    b[2] = 't';
    b[3] = 'h';
    b[4] = '3';
    b[5] = '-';
    b[6] = '6';
    b[7] = '\0';
    sync_printf("state: %d\n\n", addDevice(b));

    b[0] = 'v';
    b[1] = 'e';
    b[2] = 't';
    b[3] = 'h';
    b[4] = '6';
    b[5] = '-';
    b[6] = '3';
    b[7] = '\0';
    sync_printf("state: %d\n\n", addDevice(b));

    sync_printf("------------------------\n\n\n");
}