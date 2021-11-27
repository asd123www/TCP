#include "ringbuffer.h"


struct ringBuffer *initRingBuffer() {
    struct ringBuffer *p = (struct ringBuffer *)(malloc(sizeof(struct ringBuffer)));
    if (p == NULL) return NULL;

    p -> head = p -> tail = 0;
    p -> buffer = (u_char *)(malloc(maxRingBufferSize));
    pthread_mutex_init(&p -> lock, NULL);

    return p;
}

int __wrap_ringBufferReceiveSegment(struct ringBuffer *b, u_char *buf, int len) {
    int size = calcSize(b -> tail - b -> head);
    if (size + len >= maxRingBufferSize) return -1; // buffer overflow.

    int tmp = maxRingBufferSize - b -> tail;
    memcpy(b -> buffer + b -> tail, buf, min(tmp, len));
    if (tmp < len) {
        memcpy(b -> buffer, buf + tmp, len - tmp);
    }

    b -> tail = (b -> tail + len) %maxRingBufferSize;

    return 0;
}

int ringBufferReceiveSegment(struct ringBuffer *b, u_char *buf, int len) {
    pthread_mutex_lock(&b -> lock);
    int state = __wrap_ringBufferReceiveSegment(b, buf, len);
    pthread_mutex_unlock(&b -> lock);
    return state;
}

u_char *__wrap_ringBufferSendSegment(struct ringBuffer *b, int len) {
    int size = calcSize(b -> tail - b -> head);
    if (size < len) return NULL; // we don't have that much bytes.

    u_char *buf = (u_char *)malloc(len); // caller must free buf.
    if (buf == NULL) return NULL;

    if (b -> tail < b -> head) {
        int tmp = maxRingBufferSize - b -> head;
        memcpy(buf, b -> buffer + b -> head, min(tmp, len));
        if (len > tmp) memcpy(buf + tmp, b -> buffer, len - tmp);
    }
    else {
        memcpy(buf, b -> buffer + b -> head, len);
    }

    b -> head = (b -> head + len) % maxRingBufferSize;

    return buf;
}

u_char *ringBufferSendSegment(struct ringBuffer *b, int len) {
    pthread_mutex_lock(&b -> lock);
    u_char *state = __wrap_ringBufferSendSegment(b, len);
    pthread_mutex_unlock(&b -> lock);
    return state;
}



// int main() {
//     // first reset the maxRingBufferSize to small constant.
//     struct ringBuffer *b = initRingBuffer();

//     u_char buf[256];
//     u_char *pos, *buff;
//     int len[10]={3,2,2,3,3,3,3,3,4,2};

//     for(int i = 0; i < 256; ++i) buf[i] = i;

//     buff = buf;
//     for(int epoch = 0; epoch < 10; epoch += 2) {
//         ringBufferReceiveSegment(b, buff, len[epoch]);
//         buff += len[epoch];
//         printf("mid: head: %d   tail: %d\n", b -> head, b -> tail);
        
//         pos = ringBufferSendSegment(b, len[epoch + 1]);
//         for(int i = 0; i < len[epoch + 1]; ++i) printf("%d ", *(pos+i));
//         puts("");

//         printf("tail: head: %d   tail: %d\n\n", b -> head, b -> tail);
//         // puts("\n\n\n");
//     }
// }