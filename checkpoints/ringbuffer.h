#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <string.h>
#include <stdlib.h>



#define maxRingBufferSize 1<<15

#define min(a, b) ((a)<(b)?(a):(b))
#define max(a, b) ((a)>(b)?(a):(b))

#define calcSize(size) ((size) < 0? (size) + maxRingBufferSize: (size))


struct ringBuffer {
    int head;
    int tail;
    u_char *buffer;
    pthread_mutex_t lock; 
};

/*
 * malloc a ring buffer.
 */
struct ringBuffer *initRingBuffer();


/* @param b: the buffer we are operating on.
 * @param buf: the bytes copy into ring buffer.
 * @param len: len bytes.
 * 
 * if succeed, return 0.
 * otherwise, retur -1.
 */
int ringBufferReceiveSegment(struct ringBuffer *b, u_char *buf, int len);
int __wrap_ringBufferReceiveSegment(struct ringBuffer *b, u_char *buf, int len);


/* 
 * @param b: the buffer we are operating on.
 * @param len: is want len bytes.
 * 
 * if succeed, return the buffer with len bytes.
 * otherwise, return NULL.
 */
u_char *ringBufferSendSegment(struct ringBuffer *b, int len);
u_char *__warpper_ringBufferSendSegment(struct ringBuffer *b, int len);


#endif // RINGBUFFER_H