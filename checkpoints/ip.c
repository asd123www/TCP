
/*
 * @file ip.h
 * @brief Library supporting sending/receiving IP packets encapsulated in an Ethernet II frame. 
 */
#include <netinet/ip.h>
#include <string.h>


#include "ip.h"

// one of the .c/.cpp files: define
pthread_mutex_t setRoutingTableLock;


int setIPPacketReceiveCallback(IPPacketReceiveCallback callback) {
    IPPakcetCallback = callback;
    return 0;
}

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
    if(broadcast || !broadcast) { // asd123www, 先广播...
        ((u_char *)buffer)[0] = 0xff;
        ((u_char *)buffer)[1] = 0xff;
        ((u_char *)buffer)[2] = 0xff;
        ((u_char *)buffer)[3] = 0xff;
        ((u_char *)buffer)[4] = 0xff;
        ((u_char *)buffer)[5] = 0xff;
        return 0;
    }
    // puts("wrong in queryMacAddress!!!!");
    // exit(0);

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

    buffer[8] = 0x09; // time to live.
    buffer[9] = proto & 255;

    buffer[10] = 0;
    buffer[11] = 0; // checksum, 最后填.

    for(int i=0;i<4;++i) buffer[12+i] = (src.s_addr >> (i*8)) & 255;
    for(int i=0;i<4;++i) buffer[16+i] = (dest.s_addr >> (i*8)) & 255;

    short int *pos = (short int *)buffer;
    int sum = 0;
    for(int i = 0; i < 10; ++i) sum += *(pos + i);
    sum = (~sum) & ((1<<16) - 1);
    *(pos + 5) = sum;

    memcpy(buffer + 20, buf, len);
    queryMacAddress(dest, destmac, broadcast);

    int state = sendFrame(buffer, len + 20, 0x0800, destmac, device); // ether type = 0x0800: ipv4.

    sync_printf("packet send state: %d\n", state);
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
        for(int j = 0; j < 6; ++j) if(((u_char *)buf)[j] != subnet_list[i] -> Mac[j]) flag = 1;
        if(!flag) return i;
    }
    return -1;
}

int ipCallbackExample(const void* buf, int len, int device) {
    if (len <= 0) return 0;
    if ((*((u_char *)buf) >> 4) != 0x4) return 0; // check if it's IPv4 packet.
    if (*((u_char *)buf + 9) != 0x06) return 0; // not TCP protocol.
    
    struct in_addr src;
    struct in_addr dst;
    src.s_addr = *((uint32_t *)buf + 3);
    dst.s_addr = *((uint32_t *)buf + 4);


    int size = *((u_char *)buf + 2) << 8 | *((u_char *)buf + 3);
    assert(size == len);
    size -= (*((u_char *)buf) & 15) * 4; // minus the header length.

    return TCPPakcetCallback(buf + len - size, size, src, dst);
}

int old_ipCallbackExample(const void* buf, int len, int device) {
    #define rep(i,l,r) for(int i=l;i<=r;++i)
    if (len < 44) return 0;

    u_char *content = (u_char *)buf + 34;
    rep(i,0,9) if(*(content + i) != 66) return 0; // filter out other messages.
    content = content + 10; // point to first meaningful address.


    if (uchar2int64Mac(content)  == device_list[device] -> mac) { // the sender is device, so just drop it.
        return 0;
    }

    // 当收到广播包之后你得把它广播出去... 首先不在广播自己发出去的包.
    if (uchar2int64Mac(content)  != device_list[device] -> mac) {
        u_char *ip_header =(u_char *)buf + 14; 
        if (*(ip_header + 8) > 1) { // don't broadcast the packet whose TTL is 1.
            // sync_printf("resend broadcast!\n");
            *(ip_header + 8) = *(ip_header + 8) - 1; // TTL - 1
            *(ip_header + 10) = *(ip_header + 10) + 1; // chechsum + 1
            for(int turn_device = 0; turn_device < device_num; ++ turn_device) if (turn_device != device)
                sendFrame(buf + 14, len - 14, 0x0800, buf, turn_device); // broadcast the message.
            *(ip_header + 8) = *(ip_header + 8) + 1; // TTL - 1
            *(ip_header + 10) = *(ip_header + 10) - 1; // chechsum + 1
        }
    }
    
    // sync_printf("Received packet\n");
    // rep(i,0,39) sync_printf("%02x ",((u_char *)buf)[i]);
    // sync_printf("\n");

    struct in_addr ip_addr;
    struct in_addr mask; mask.s_addr = ~0;
    ip_addr.s_addr = (((u_char *)buf)[26] << 24) | (((u_char *)buf)[27] << 16) | (((u_char *)buf)[28] << 8) | ((u_char *)buf)[29];

    setRoutingTable(ip_addr, mask, content, buf + 6, buf);

    int idx = findMacAddress(buf + 6);
    if(idx != -1) { // have that point, and explore the content of the message.
        // sync_printf("set edge\n");
        setEdgeEntry(1, 0, idx + 1); // get directly connected MAC address, update the router edge.
        idx = findMacAddress(content);
        // if(idx == -1) {
        //     sync_printf("wrong!\n");
        //     exit(0);
        //     return 0;
        // }

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

    pthread_mutex_lock(&setRoutingTableLock);

    int idx = findIPAddress(dest, mask);
    if (idx != -1) { // 加锁之后不能随便退出！！！
        memcpy(subnet_list[idx] -> Mac, (u_char *)MAC, 8);
        memcpy(subnet_list[idx] -> nextHopMac, (u_char *)nextHopMAC, 8);
    }
    else { // 如果这里面return就会发生deadlock, 没有释放锁.
        struct subnet *ptr = (struct subnet *)malloc(sizeof(struct subnet));

        if(ptr == NULL) return -1;
        ptr -> addr = dest.s_addr;
        ptr -> mask = mask.s_addr;
        ptr -> Mac = (u_char *)malloc(10);
        ptr -> nextHopMac = (u_char *)malloc(10);
        if(ptr -> Mac == NULL || ptr -> nextHopMac == NULL) return -1;
        memcpy(ptr -> Mac, (u_char *)MAC, 8);
        memcpy(ptr -> nextHopMac, (u_char *)nextHopMAC, 8);
        subnet_list[subnet_num++] = ptr;
    }

    pthread_mutex_unlock(&setRoutingTableLock);
    return 0;
}