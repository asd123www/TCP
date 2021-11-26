/**
 * @file packetio.h
 * @brief Library supporting sending/receiving Ethernet II frames. 
 */
#include <stdio.h>
#include <stdlib.h>
#include <netinet/ether.h>
#include <time.h>

#include "device.h"
/**
 * @brief Encapsulate some data into an Ethernet II frame and send it.
 * @param buf Pointer to the payload.
 * @param len Length of the payload.
 * @param ethtype EtherType field value of this frame.
 * @param destmac MAC address of the destination.
 * @param id ID of the device(returned by 'addDevice') to send on.
 * @return 0 on success, -1 on error.
 * @see addDevice
 */
int sendFrame(const void* buf, int len, int ethtype, const void* destmac, int id);

int open_pcap_dev(pcap_t **result, const char* name, char* errbuf);


/**
 * @brief Process a frame upon receiving it.
 * @param buf Pointer to the frame.
 * @param len Length of the frame.
 * @param id ID of the device (returned by 'addDevice') receiving current frame.
 * @return 0 on success, -1 on error.
 * @see addDevice 
 */
typedef int (*frameReceiveCallback)(const void*, int, int);

/**
 * @brief Register a callback function to be called each time an Ethernet II frame was received.
 * @param callback the callback function.
 * @param id ID of the device(returned by 'addDevice') to receive.
 * @return 0 on success, -1 on error.
 * @see frameReceiveCallback 
 */
int setFrameReceiveCallback(frameReceiveCallback callback, int id);

int frameCallbackExample(const void *buf, int len, int id);

/*
 * @brief Process an IP packet upon receiving it.
 * 
 * @param buf Pointer to the packet. 
 * @param len Length of the packet.
 * @return 0 on success, -1 on error.
 * @see addDevice 
 */
typedef int (*IPPacketReceiveCallback)(const void* buf, int len, int device);
IPPacketReceiveCallback IPPakcetCallback;