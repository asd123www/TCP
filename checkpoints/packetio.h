/**
 * @file packetio.h
 * @brief Library supporting sending/receiving Ethernet II frames. 
 */
#include <stdio.h>
#include <stdlib.h>
#include <netinet/ether.h>

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


/*   +---------------------+---------------------+---------------+------+----------------+
 *   | dst MAC address(6B) | src MAC address(6B) | EtherType(2B) | data | checksum(undo) |
 *   +---------------------+---------------------+---------------+------+----------------+
 */
int sendFrame(const void* buf, int len, int ethtype, const void* destmac, int id) {
    char errbuf[PCAP_ERRBUF_SIZE];

    pcap_t *handle;
    open_pcap_dev(&handle, device_list[id] -> name, errbuf);

    u_char *buffer = (u_char *)malloc(len + 14);
    if (buffer == NULL) {
        fprintf(stderr,"\nUnable to malloc the buffer.\n");
        return -1;
    }
    for (int i = 0; i < 6; ++i) buffer[i] = *((u_char *)destmac + i);
    for (int i = 6; i < 12; ++i) buffer[i] = (device_list[id] -> mac >> 8*(11-i)) & 255;
    
    for (int i = 0; i < 6; ++i) printf("%02x.", buffer[i]);
    printf("\n");



    buffer[12] = (ethtype >> 8) & 255;
    buffer[13] = ethtype & 255;
    for (int i = 0; i < len; ++i) buffer[i+14] = *((u_char *)buf + i);

    if(pcap_sendpacket(handle, buffer, len + 14) != 0) { // send err.
        fprintf(stderr,"\nSend packets error.\n");
        return -1;
    }
    return 0;
}

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


/* typedef void (*pcap_handler)(u_char *, const struct pcap_pkthdr *, const u_char *);
 * int pcap_loop(pcap_t *p, int cnt, pcap_handler callback, u_char *user);
 * the first param in pcap_handler is the last param in pcap_loop. 
 * the second param is pcap header, which contains information about when the packet was sniffed, how large it is, etc.
 * 
 * struct pcap_pkthdr {
 *     struct timeval ts;	// time stamp
 *     bpf_u_int32 caplen;	// length of portion present
 *     bpf_u_int32 len;	    // length this packet (off wire)
 * };
 * 
 * The last param points to the first byte of a chunk of data containing the entire packet.
 * The buffer!
 * 
 * int pcap_next_ex(pcap_t *p, struct pcap_pkthdr **pkt_header, const u_char **pkt_data);
 * 
 */
int setFrameReceiveCallback(frameReceiveCallback callback, int id) {
    char errbuf[PCAP_ERRBUF_SIZE];
    u_char *buf;
    struct pcap_pkthdr *header;
    pcap_t *handle;
    open_pcap_dev(&handle, device_list[id] -> name, errbuf);

    // printf("%d\n", pcap_next_ex(handle, &header, (const u_char **)&buf));
    int x = PCAP_ERROR;
    while (pcap_next_ex(handle, &header, (const u_char **)&buf) == 1) {
        callback(buf, header -> len, id);
        return 0;
    }
    return -1;
}


int callback_example(const void *buf, int len, int id) {
    #define rep(i,l,r) for(int i=l;i<=r;++i)
    printf("Dst address: ");
    rep(i,0,5) printf("%02x.", *((u_char *)buf+i));
    puts("");
    printf("Src address: ");
    rep(i,0,5) printf("%02x.", *((u_char *)buf+i));
    puts("");
    printf("data length: %d\n", len-14);
    puts("");
}


int open_pcap_dev(pcap_t **result, const char* name, char* errbuf) {
	pcap_t* handle = pcap_create(name, errbuf);
	if (handle) {
        // pcap_set_rfmon(handle, 1);
        // pcap_set_snaplen(handle, 2048);
        // pcap_set_promisc(handle, 1);
        // pcap_set_timeout(handle, 512);
        
        int status;
        status = pcap_activate(handle);
        // printf("wrong status: %d\n", status);
        
        // char *err = pcap_geterr(handle);
        // for (int i=0;i<50;++i)
        //     putchar(*(err+i));
        // puts("");
	}
    *result = handle;
    return 0;
}