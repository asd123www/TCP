#ifndef DEVICE_H
#define DEVICE_H


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

#define MAX_DEVICE_NUMBER 100
char errbuf[PCAP_ERRBUF_SIZE];


extern pthread_mutex_t printfMutex;

struct net_device {
    char *name;
    uint32_t id;
    uint32_t ip;
    uint64_t mac;
};

int device_num;
struct net_device *device_list[MAX_DEVICE_NUMBER];

uint64_t uchar2int64Mac(u_char *buf);

int sync_printf(const char *format, ...);

void test(); // add the devices needed.

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
int findDevice(const char *device);

#endif // DEVICE_H