/**
 * @file packetio.h
 * @brief Library supporting sending/receiving Ethernet II frames. 
 */
#include <stdio.h>
#include <stdlib.h>
#include <netinet/ether.h>
#include <time.h>

#include "packetio.h"

/*   +---------------------+---------------------+---------------+------+----------------+
 *   | dst MAC address(6B) | src MAC address(6B) | EtherType(2B) | data | checksum(undo) |
 *   +---------------------+---------------------+---------------+------+----------------+
 */
int sendFrame(const void* buf, int len, int ethtype, const void* destmac, int id) {

    // emulate lossy channel.
    if (rand()%5 < 2) return 0;


    char errbuf[PCAP_ERRBUF_SIZE];

    pcap_t *handle;
    // printf("sender: %s\n", device_list[id] -> name);
    open_pcap_dev(&handle, device_list[id] -> name, errbuf);

    u_char *buffer = (u_char *)malloc(len + 14);
    if (buffer == NULL) {
        // fprintf(stderr,"\nUnable to malloc the buffer.\n");
        return -1;
    }

    // for (int i = 0; i < 6; ++i) sync_printf("%02x.", buffer[i]);
    // sync_printf("\n");

    for (int i = 0; i < 6; ++i) buffer[i] = *((u_char *)destmac + i);
    for (int i = 6; i < 12; ++i) buffer[i] = (device_list[id] -> mac >> 8*(11-i)) & 255;
    
    // for (int i = 6; i < 12; ++i) sync_printf("%02x.", buffer[i]);
    // sync_printf("\n");

    buffer[12] = (ethtype >> 8) & 255;
    buffer[13] = ethtype & 255;
    for (int i = 0; i < len; ++i) buffer[i+14] = *((u_char *)buf + i);

    // sync_printf("length: %d\n", len);
    // for (int i = 0; i < 34; ++i)  sync_printf("%02x ", buffer[i]);

    if(pcap_sendpacket(handle, buffer, len + 14) != 0) { // send err.
        // fprintf(stderr,"\nSend packets error.\n");
        return -1;
    }
    pcap_close(handle);
    free(buffer);

    // printf("have sent the packet.\n");
    return 0;
}


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
    
    // int x = PCAP_ERROR;
    // int pcap_set_timeout(pcap_t *p, int to_ms);

    while (1) {
        int state = pcap_next_ex(handle, &header, (const u_char **)&buf);
        if(state != 1) continue;

        // sync_printf("packet received\n");

        callback(buf, header -> len, id);
        // sync_printf("packet processed!!!\n");
        // sync_printf("");
    }
    sync_printf("wrong !!!\n");
    exit(0);
    pcap_close(handle);

    return -1;
}

int frameCallbackExample(const void *buf, int len, int id) {
    #define rep(i,l,r) for(int i=l;i<=r;++i)
    // printf("Dst mac address: ");
    // rep(i,0,5) printf("%02x.", *((u_char *)buf+i));
    // puts("");

    // printf("%lx\n", (uint64_t)(-1));
    // printf("dst:%lx  device:%lx\n\n\n", uchar2int64Mac((u_char *)buf), device_list[id] -> mac);
    // printf("Src mac address: ");
    // rep(i,6,11) printf("%02x.", *((u_char *)buf+i));
    // puts("");
    // uchar2int64Mac(content)  != device_list[device] -> mac
    if (uchar2int64Mac((u_char *)buf + 6)  == device_list[id] -> mac) { // the sender is device, so just drop it.
        return 0;
    }

    // printf("received packet. %ld \n", uchar2int64Mac((u_char *)buf));
    if (uchar2int64Mac((u_char *)buf) == 0xffffffffffffll || 
        uchar2int64Mac((u_char *)buf) == device_list[id] -> mac)  // the mac address matches.
        IPPakcetCallback(buf + 14, len - 14, id);
        
    return 0;
}

int open_pcap_dev(pcap_t **result, const char* name, char* errbuf) {
	pcap_t* handle = pcap_create(name, errbuf);
    pcap_set_immediate_mode(handle,1);
	if (handle) {
        // pcap_set_rfmon(handle, 1);
        // pcap_set_snaplen(handle, 2048);
        // pcap_set_promisc(handle, 1);
        // pcap_set_timeout(handle, 512);
        if (pcap_set_buffer_size(handle, (1<<20)) != 0) {
            printf("set buffer size error.\n");
            return -1;
        }
        
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