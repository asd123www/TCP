/*
 * the implementation of basic route algorithms.
 */
#include <stdio.h>
#include <stdlib.h>
#include <netinet/ip.h>
#include <string.h>


#include "route.h"


pthread_mutex_t edgeEntryLock;

void setEdgeEntry(int value, int i, int j) {
    pthread_mutex_lock(&edgeEntryLock);
    if(i == -1 && j == -1) {
        memset(edge, value, sizeof(edge));
        // for(int i = 0; i < MAX_ROUTER_NUM; ++i) edge[i][i] = 0;
    }
    else {
        edge[i][j] = value;
        edge[j][i] = value;
    }
    pthread_mutex_unlock(&edgeEntryLock);
}

void Dijkstra(int n) {
    // if(edge[0][0] != 0) return;  // no update.
    memset(dist, 0x3f, sizeof(dist));
    for(int i = 0; i < n; ++i) {
        for(int j = 0; j < n; ++j) 
            dist[i][j] = edge[i][j];
        dist[i][i] = 0;
    }
    for(int k = 0; k < n; ++k) 
        for(int i = 0; i < n; ++i) 
            for(int j = 0; j < n; ++j) 
                dist[i][j] = min(dist[i][j], dist[i][k] + dist[k][j]);
    return;
}