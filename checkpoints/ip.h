
#ifndef IP_H
#define IP_H
/*
 * @file ip.h
 * @brief Library supporting sending/receiving IP packets encapsulated in an Ethernet II frame. 
 */
#include <netinet/ip.h>
#include <string.h>
#include <assert.h>

#include "ip_layer.h"
#include "route.h"
#include "packetio.h"

int subnet_num;

// .h header file: declare
extern pthread_mutex_t setRoutingTableLock;


// pthread_mutex_t setRoutingTableLock = PTHREAD_MUTEX_INITIALIZER;
struct subnet *subnet_list[MAX_ROUTER_NUM + 5];

int queryMacAddress(const struct in_addr dest, const void *buf, int broadcast);




typedef int (*TCPPacketReceiveCallback)(const void* buf, int len, struct in_addr src, struct in_addr dst);
TCPPacketReceiveCallback TCPPakcetCallback;

/* 
 * easy implement of longest prefix match algo.
 * time complexity is O(n), n is the # of subnets.
 * easy to optimize to O(\log n) by 0/1 trie, but not necessary.
 */
struct subnet* longestPrefixMatch(uint32_t ip);

/*
 * @brief Send an IP packet to specified host.
 *
 * @param src: Source IP address.
 * @param dest: Destination IP address.
 * @param proto: Value of 'protocol' field in IP header.
 * @param buf: pointer to IP payload.
 * @param len: Length of IP payload.
 * @param broadcast: if link layer need broadcast.
 * @return 0 on success, -1 on error. 
 * 
 * 我们调用sendframe时要给确定的dest MAC address, 使用ARP(address resolution protocol)询问即可.
 */
int sendIPPacket(const struct in_addr src, const struct in_addr dest, int proto, const void *buf, int len, int broadcast, int device);

/*
 * @brief Register a callback function to be called each time an IP packet was received.
 * 
 * @param callback The callback function.
 * @return 0 on success, -1 on error.
 * @see IPPacketReceiveCallback
 */
int setIPPacketReceiveCallback(IPPacketReceiveCallback callback);
int ipCallbackExample(const void* buf, int len, int device);


int findIPAddress(struct in_addr dest, struct in_addr mask);
int findMacAddress(const void *buf);

/*
 * @brief Manully add an item to routing table. Useful when talking with real Linux machines. 
 *
 * @param dest The destination IP prefix.
 * @param mask The subnet mask of the destination IP prefix.
 * @param nextHopMAC MAC address of the next hop.
 * @param device Name of device to send packets on.
 * @return 0 on success, -1 on error 
 */
int setRoutingTable(const struct in_addr dest, const struct in_addr mask, const void *MAC, const void* nextHopMAC , const char *device);

#endif // IP_H