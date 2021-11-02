
/*
 * @file ip.h
 * @brief Library supporting sending/receiving IP packets encapsulated in an Ethernet II frame. 
 */
#include "route.h"
#include "packetio.h"
#include <netinet/ip.h>
#include <string.h>


int subnet_num;
pthread_mutex_t setRoutingTableLock = PTHREAD_MUTEX_INITIALIZER;
struct subnet *subnet_list[MAX_ROUTER_NUM + 5];

int queryMacAddress(const struct in_addr dest, const void *buf, int broadcast);

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
int setIPPacketReceiveCallback(IPPacketReceiveCallback callback) {
    IPPakcetCallback = callback;
    return 0;
}


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



struct subnet* longestPrefixMatch(uint32_t ip) {
    struct subnet *ans = NULL;
    for(int i = 0; i < subnet_num; ++i) {
        struct subnet *idx = subnet_list[i];
        if ((idx -> addr & idx -> mask) != (ip & idx -> mask)) continue;
        if (ans == NULL || ans -> length < idx -> length) ans = idx;
    }
    return ans;
}

int queryMacAddress(const struct in_addr dest, const void *buffer, int broadcast) {
    if(broadcast) {
        ((u_char *)buffer)[0] = 0xff;
        ((u_char *)buffer)[1] = 0xff;
        ((u_char *)buffer)[2] = 0xff;
        ((u_char *)buffer)[3] = 0xff;
        ((u_char *)buffer)[4] = 0xff;
        ((u_char *)buffer)[5] = 0xff;
        return 0;
    }
    puts("wrong in queryMacAddress!!!!");
    exit(0);

    struct subnet* idx = longestPrefixMatch(dest.s_addr);
    if (idx != NULL) {
        memcpy((u_char *)buffer, idx -> nextHopMac, 6);
        return 0;
    }
    return -1;
}


int sendIPPacket(const struct in_addr src, const struct in_addr dest, int proto, const void *buf, int len, int broadcast, int device) {
    if (len > 65516) {
        sync_printf("too many bytes in IP packet!\n");
        return -1;
    }
    static short int identification;
    u_char *destmac = (u_char *)malloc(10);
    u_char *buffer = (u_char *)malloc(len + 20);
    if (destmac == NULL || buffer == NULL) {
        sync_printf("Can't malloc the space needed!\n");
        return -1;
    }

    buffer[0] = 0x45; // ip version 4 AND without options is 5 32-bit words.
    buffer[1] = 0x00; // don't need! PRECEDENCE = 0x000 (routine), Delay = 0, Throughput = 0, Reliability = 0.
    buffer[2] = (len + 20) >> 8;
    buffer[3] = (len + 20) & 255; // the length of the packet is 20 + len.
    ++identification;
    buffer[4] = identification >> 8;
    buffer[5] = identification & 255;

    buffer[6] = 0x40;
    buffer[7] = 0x00; // set don't fragment, others zero.

    buffer[8] = 0x40; // time to live.
    buffer[9] = proto & 255;

    buffer[10] = 0;
    buffer[11] = 0; // checksum, 最后填.

    for(int i=0;i<4;++i) buffer[12+i] = (src.s_addr >> ((3-i)*8)) & 255;
    for(int i=0;i<4;++i) buffer[16+i] = (dest.s_addr >> ((3-i)*8)) & 255;

    short int *pos = (short int *)buffer;
    int sum = 0;
    for(int i = 0; i < 10; ++i) sum += *(pos + i);
    sum = (~sum) & ((1<<16) - 1);
    *(pos + 5) = sum;

    memcpy(buffer + 20, buf, len);
    queryMacAddress(dest, destmac, broadcast);

    int state = sendFrame(buffer, len + 20, 0x0800, destmac, device); // ether type = 0x0800: ipv4.

    // sync_printf("packet send state: %d\n", state);
    free(destmac);
    free(buffer);

    return state;
}


int findIPAddress(struct in_addr dest, struct in_addr mask) {
    for (int i = 0; i < subnet_num; ++i) {
        if (dest.s_addr == subnet_list[i] -> addr && mask.s_addr == subnet_list[i] -> mask) {
            return i;
        }
    }
    return -1;
}
int findMacAddress(const void *buf) {
    for (int i = 0; i < subnet_num; ++i) {
        int flag = 0;
        for(int j = 0; j < 6; ++j) if(((u_char *)buf)[j] == subnet_list[i] -> Mac[j]) flag = 1;
        if(!flag) return i;
    }
    return -1;
}


int ipCallbackExample(const void* buf, int len, int device) {
    #define rep(i,l,r) for(int i=l;i<=r;++i)
    if (len < 44) return 0;

    u_char *content = (u_char *)buf + 34;
    content = content + 10; // point to first meaningful address.
    rep(i,0,9) if(*(content + i) != 66) return 0; // filter out other messages.

    sync_printf("Received packet\n");
    rep(i,0,39) sync_printf("%02x ",((u_char *)buf)[i]);
    sync_printf("\n");

    // 当收到广播包之后你得把它广播出去...
    if (uchar2int64Mac(content)  != device_list[device] -> mac)
        sendFrame(buf + 14, len - 14, 0x0800, buf, device); // broadcast the message.


    struct in_addr ip_addr;
    struct in_addr mask; mask.s_addr = ~0;
    ip_addr.s_addr = (((u_char *)buf)[26] << 24) | (((u_char *)buf)[27] << 16) | (((u_char *)buf)[28] << 8) | ((u_char *)buf)[29];



    setRoutingTable(ip_addr, mask, content, buf + 6, buf);

    int idx = findMacAddress(buf + 6);
    if(idx != -1) { // have that point, and explore the content of the message.
        sync_printf("set edge\n");
        setEdgeEntry(1, 0, idx + 1); // get directly connected MAC address, update the router edge.
        content += 6; // other MAC addresses directly connected to idx.
        for(int i = 50, j; i < len; i += 6) {
            j = findMacAddress(content);
            if(j != -1) setEdgeEntry(1, idx + 1, j+1);
            content += 6;
        }
    }
    // sync_printf("ip: %d\n", ip_addr.s_addr);
    // sync_printf("total number: %d\n", subnet_num);

    return 0;
}

int setRoutingTable(const struct in_addr dest, const struct in_addr mask, const void *MAC, const void* nextHopMAC , const char *device) {
    /* 是否要加一个默认的longest match algo的entity?
    if(subnet_num == 0) {
        struct subnet *ptr = (struct subnet *)malloc(sizeof(struct subnet));
        if(ptr == NULL) {
            return -1;
        }
        ptr -> addr = 0;
        ptr -> mask = 0;
        ptr -> nxt = NULL;
        subnet_list[0] = ptr;
        ++subnet_num;
    }
    */

    pthread_mutex_lock(&setRoutingTableLock);

    int idx = findIPAddress(dest, mask);
    if (idx != -1) { // 加锁之后不能随便退出！！！
        memcpy(subnet_list[idx] -> Mac, (u_char *)MAC, 8);
        memcpy(subnet_list[idx] -> nextHopMac, (u_char *)nextHopMAC, 8);
    }
    else { // 如果这里面return就会发生deadlock, 没有释放锁.
        struct subnet *ptr = (struct subnet *)malloc(sizeof(struct subnet));

        if(ptr == NULL) return -1;//
        ptr -> addr = dest.s_addr;
        ptr -> mask = mask.s_addr;
        ptr -> Mac = (u_char *)malloc(10);
        ptr -> nextHopMac = (u_char *)malloc(10);//
        if(ptr -> Mac == NULL || ptr -> nextHopMac == NULL) return -1;
        memcpy(ptr -> Mac, (u_char *)MAC, 8);
        memcpy(ptr -> nextHopMac, (u_char *)nextHopMAC, 8);
        subnet_list[subnet_num++] = ptr;
    }

    pthread_mutex_unlock(&setRoutingTableLock);
    return 0;
}