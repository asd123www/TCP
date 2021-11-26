/*
 * Some hyper parameters used in network layer, eg IP. 
 */
#include <netinet/ether.h>

#define MAX_ROUTER_NUM 100

#define subnet_prefix(len) (((1 << len) - 1) << (32 - len))

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
// #define subnet_prefix(x, len) \
//     do { (x & (((1 << len) - 1) << (32 - len))) } while(0)

struct subnet {
    int length;
    uint32_t addr;
    uint32_t mask;
    u_char *Mac;
    u_char *nextHopMac;
};
