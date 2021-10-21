/**
 * @file device.h
 * @brief Library supporting network device management. 
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pcap/pcap.h>

char errbuf[PCAP_ERRBUF_SIZE];

struct net_device {
    char *name;
    uint32_t id;
    uint32_t ip;
    uint64_t mac;
    struct net_device *nxt;
};

struct net_device *device_list;

/**
 * Add a device to the library for sending/receiving packets.
 * @param device Name of network device to send/receive packet on.
 * @return A non-negative _device-ID_ on success, -1 on error. 
 */
int addDevice(const char *device);

/**
 * Find a device added by 'addDevice'.
 * @param device Name of the network device.
 * @return A non-negative _device-ID_ on success, -1 if no such device
 * was found.
 */
struct net_device* findDevice(const char *device);

struct net_device* findDevice(const char *device) {
    for (struct net_device *iter = device_list; iter != NULL; iter = iter -> nxt) {
        if (strcmp(iter -> name, device) != 0) continue;
        return iter;
    }
    return NULL;
}

int addDevice(const char* device) {
    static pcap_if_t *head;
    // struct pcap_addr t;
    if (head == NULL) {
        int state = pcap_findalldevs(&head, errbuf);
        if (state == -1) return -1;
    }

    for (pcap_if_t *iter = head; iter != NULL; iter = iter->next) {
        if (strcmp(iter->name, device) != 0) continue;

        // printf("name: %s\n", iter->name);
        struct net_device *ptr = (struct net_device *)malloc(sizeof(struct net_device));
        ptr -> name = (char *)malloc(sizeof(device));
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
                for (int j=10; j<16; ++j) {
                    ptr -> mac <<= 8;
                    ptr -> mac |= i->addr->sa_data[j] & 255;
                }

                // printf("address: ");
                // for (int j=1; j<7; ++j) {
                //     printf("%02lx.", ptr->mac >> ((6-j)*8) & 255);
                // }
                // puts("");
            }
        }

        ptr -> nxt = NULL;
        if (device_list == NULL) device_list = ptr;
        else {
            // if I maintain the last, then the device id is kind of awkward.
            for (struct net_device *i = device_list; i != NULL; i = i -> nxt) { 
                if (i -> nxt == NULL) {
                    i -> nxt = ptr;
                    break;
                }
            }
        }
        return 1;
    }
    return -1;
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
    printf("state: %d\n\n", addDevice(a));

    b[0] = 'v';
    b[1] = 'i';
    b[2] = 'r';
    b[3] = 'b';
    b[4] = 'r';
    b[5] = '0';
    b[6] = '\0';
    printf("name: %s\n",b);
    printf("state: %d\n\n", addDevice(b));

    printf("------------------------\n\n\n");

    printf("name: %s\n", findDevice(a)->name);
    printf("name: %s\n", findDevice(b)->name);
}