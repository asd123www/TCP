#ifndef ROUTE_H
#define ROUTE_H
/*
 * the implementation of basic route algorithms.
 */
#include <stdio.h>
#include <stdlib.h>
#include <netinet/ip.h>
#include <string.h>
#include <pthread.h>

#include "ip_layer.h"


extern pthread_mutex_t edgeEntryLock;

int edge[MAX_ROUTER_NUM][MAX_ROUTER_NUM];
int dist[MAX_ROUTER_NUM][MAX_ROUTER_NUM];

/*
 * floyd algorithm find shortest path.
 */
void Dijkstra(int n);
void setEdgeEntry(int value, int i, int j);


#endif // ROUTE_H