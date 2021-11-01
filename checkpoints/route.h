/*
 * the implementation of basic route algorithms.
 */
#include <stdio.h>
#include <stdlib.h>
#include "ip_layer.h"
#include <netinet/ip.h>
#include <string.h>


pthread_mutex_t edgeEntryLock;
int edge[MAX_ROUTER_NUM][MAX_ROUTER_NUM];
int dist[MAX_ROUTER_NUM][MAX_ROUTER_NUM];

/* 
 * floyd algorithm find shortest path.
 *
 */
void Dijkstra(int n);

/* 
 * easy implement of longest prefix match algo.
 * time complexity is O(n), n is the # of subnets.
 * easy to optimize to O(\log n) by 0/1 trie, but not necessary.
 */
struct subnet* longestPrefixMatch(struct subnet *lst, uint32_t ip);


void setEdgeEntry(int value, int i, int j) {
    pthread_mutex_lock(&edgeEntryLock);
    if(i == -1 && j == -1) {
        memset(edge, value, sizeof(edge));
        for(int i = 0; i < MAX_ROUTER_NUM; ++i) edge[i][i] = 0;
    }
    else {
        edge[i][j] = value;
        edge[j][i] = value;
    }
    pthread_mutex_unlock(&edgeEntryLock);
}

void Dijkstra(int n) {
    memset(dist, 0x3f, sizeof(dist));
    for(int i = 0; i < n; ++i)
        for(int j = 0; j < n; ++j) 
            dist[i][j] = edge[i][j];
    for(int k = 0; k < n; ++k) 
        for(int i = 0; i < n; ++i) 
            for(int j = 0; j < n; ++j) 
                dist[i][j] = min(dist[i][j], dist[i][k] + dist[k][j]);
    return;
}

struct subnet* longestPrefixMatch(struct subnet *lst, uint32_t ip) {
    struct subnet *ans = NULL;
    for(struct subnet *idx = lst; idx != NULL; idx = idx -> nxt) {
        if ((idx -> addr & idx -> mask) != (ip & idx -> mask)) continue;
        if (ans == NULL || ans -> length < idx -> length) ans = idx;
    }
    return ans;
}