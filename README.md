# Transport-layer: TCP Protocol

## how to run the code

In `/checkpoint`, you can run `make` to compile the source code.

For the end host, run `make test-echo_server` for example. The `networkStack` is for middle points to route packets.



### source code

`ringBuffer.h`/`ringbuffer.c`: implement the ring buffer.

`tcp.h`/`tcp.c`: realize the TCP based on ringbuffer and lower-layer function.

`socket.h`/`socket.c`: realize the standard socket API.



## My TCP implementation.

### ringBuffer

For buffering the sending/receiving packets, a buffer is necessary. Therefore I choose the most efficient algorithm (I think) — ring buffer.

```c
struct ringBuffer {
    int head;
    int tail;
    u_char *buffer;
    pthread_mutex_t lock; 
};
```

It's easy to realize a ring buffer with locks. However I learned that a lock-free ring buffer is 8-10 times faster than mine, therefore I  take it as a future optimization bottleneck.

### TCP

Based on the [RFC793](https://datatracker.ietf.org/doc/html/rfc793), I maintained the `tcb` and `header`. The ring buffer `rx,tx` is necessary. The RTT is a fixed variable which is 1 second. `wantClose` indicates whether the socket calls `close` or not. `lastClock` records the last transmit time for retransmission. `lock` is for sender/receiver thread synchronization.

```c
struct tcpInfo {
    pthread_mutex_t lock; // syn for send/receive.
    int wantClose;

    uint64_t lastClock;
    uint64_t RTT;
  
    struct tcpHeader *h;
    struct ringBuffer *rx;
    struct ringBuffer *tx;

    struct TransmissionControlBlock tcb;
};
```

For the existence of unacked data, we should use a ring buffer to store the data. 

```c
        1           2         3          4 
    ----------|----------|----------|---------- 
           SND.UNA    SND.NXT    SND.UNA
                                +SND.WND
    1 - old sequence numbers which have been acknowledged
    2 - sequence numbers of unacknowledged data
    3 - sequence numbers allowed for new data transmission
    4 - future sequence numbers which are not yet allowed Send Sequence Space
```

Then the TCP is implemented just following the RFC793, but massive bugs have to be fixed.

### socket

The socket and the TCP connection are explicitly distinguished in my design. When a socket is allocated, there are two types of results:

1. The server `listen`: just allocate `backlog` tcps set to state `LISTEN`.
2. The client `connect`: just allocate a tcp to replace that socket.

Though obviously, the server will waste a large number of resources, it greatly simplified my code complexity.

Whenever a connection is set, I create a thread for that tcp connection for sending packets.

Each device has a receiver thread whose duty is delivering the corresponding packet to the tcp connection.



## Corner cases:

### Retransmission:

It's easy to have a `lastClock` variable record the last transmitted packet's time. Every time before sending a packet, check whether there is a timeout or not.

```c
if (tmp - tcp -> lastClock < tcp -> RTT && tcp -> lastClock) { // didn't timeout.
  	state = -1; // give up the lock.
}
else { // we need to send another syn message.
    tcp -> lastClock = tmp;
    state = __wrap_TCP2IPSender(tcp -> h, tcp -> tcb.SND.ISS, 0, TCP_SYN, tcp -> tcb.RCV.WND, 0, buf, 0);
    tcp -> tcb.SND.NXT = tcp -> tcb.SND.ISS + 1;
}
```


### TCP_FINWAIT_2

After receiving the FIN, you sent the ACK, but that packet may be lost in the network. Therefore you have to take extra care of that situation. 

Here my solution is to shut down the sender thread for a specific period, if during this time interval a retransmitted `FIN` packet is received, then start another shutdown period, otherwise close.

```c
case TCP_TIME_WAIT:
    if (tcp -> wantClose != 2) {
        state = 2;
        tcp -> wantClose = 2;
    }
    else {
        tcp -> tcb.tcpState = TCP_CLOSE;
        state = 0;
    }
    break;
```

Every time we set the `wanClose` to 2, if received a packet, the receiver will change it to 1.

### The meaningless ACK packet

At the beginning of my implementation, I found mass ACK packets. I found the reason that this happened for my ACK to an ACK, whose length is zero. Just a special judge is OK.

### The duplicated or out-of-window packet

Take advantage of the `unsigned int`, check if it's in the window.

```c
if (ackNumber - tcp ->tcb.SND.UNA > INITIAL_WINDOW_SIZE) return; // not the packet I want.
```





## checkpoints:

### Writing Task 3 (WT3). Describe how you correctly handled TCP state changes.

My state changes are based on the DFA described in the [RFC793](https://datatracker.ietf.org/doc/html/rfc793).

After pondering on the state changes, I found only three types of actions that may cause the state changing: sending a packet, receiving a packet, and the actions in the socket. Therefore this is natural to maintain separately a sender function and a receiver function to handle state changes meanwhile implementing the basic TCP functions. For the actions in socket is simple compared to sender/receiver, I implemented a variable in `struct tcpInfo` named `wantClose` to indicate whether the socket called the `close ` or not. It has another use in `FIN`.

The receiver can only send the ACKs for eviting retransmission. All other packet sending tasks is offloaded to the sender. I think this design greatly reduced the code complexity.

The sender/receiver's structure is like:

```c
void* tcpSender(void *pt) {
    while (1) {
        pthread_mutex_lock(&tcp -> lock);
        switch (tcp -> tcb.tcpState) {
        case LISTEN:
            break;
        
        case TCP_SYN_SENT:
            break;

        case TCP_SYN_RECV:
            break;
            
        case TCP_FIN_WAIT1:
            break;

        case TCP_FIN_WAIT2:
            break;

        case TCP_CLOSING: 
            break;

        case TCP_CLOSE_WAIT: 
            break;

        case TCP_LAST_ACK:
            break;

        case TCP_CLOSE:
            break;

        case TCP_ESTABLISHED: // connection established
            break;

        default:
            break;
        }
        pthread_mutex_unlock(&tcp -> lock);
    }
}
```

First, the sender/receiver may both change the TCP state, so the TCP connection unique lock is for synchronization. The `switch` distinguishes different states for different actions.

The state changing in sender/receiver is straightforward. For instance:

```c
case TCP_ESTABLISHED: // connection established
    tmp = clock();
    if (tcp -> wantClose == 1) {
      // 发送FIN.
        tcp -> lastClock = tmp;
        state = __wrap_TCP2IPSender();
        tcp -> tcb.SND.NXT ++;
        tcp -> tcb.tcpState = TCP_FIN_WAIT1;
    }
    else {
        if (tmp - tcp -> lastClock > tcp -> RTT && tcp -> lastClock) { // timeout
            tcp -> tcb.SND.NXT = tcp -> tcb.SND.UNA; // retransmit the first byte.
        }
        state = TcpSlidingWindowSender(tcp, TCP_ACK);
        if (!state) tcp -> lastClock = tmp; // update the timer.
    }
    break;
```

As mentioned above, `wantClose` indicates whether the TCP wants to close or not. If it wants to close, then just send the `FIN` and change the state.



### Checkpoint 7 (CP7). 

![Screen Shot 2021-12-05 at 4.50.56 PM](/Users/asd123www/Library/Application Support/typora-user-images/Screen Shot 2021-12-05 at 4.50.56 PM.png)

The starting 34 bytes are link-layer and network-layer headers.

The `66 27` indicates source port: 26151.

The `c5 67` indicates destination port: 50535.

The `00 00 25 bd` indicates sequence number: 9661.

The `00 00 2d a5` indicates acknowledgment number: 11685.

The `50 10` indicates the length and bit flags.

The `ff ff` means the window size is 65535.

The `33 fe` is the checksum, and `00 00 ` is the urgent pointer.



### Checkpoint 8 (CP8)

#### The lossy/reordered/duplicated/out-of-window packets environment

For the lossy network, I simply did this in my link-layer sender:

```c
if (rand()%5 < 2) return 0;
```

Thus I think it's equal to `40%` loss rate channel.

The reordered/duplicated/out-of-window packets are natural when your router just broadcasts all the packets to all ports until the TTL goes to zero.  I think it's called the packet storm. 

### My result

I tested `echo` and `perf` in such an environment, both of them successfully sent/received packets. The most challenging part is the breakup process. It's full of details that may cause one side or both sides' hosts' TCP connection to stay alive permanently and continuously retransmit packets.

![Screen Shot 2021-12-05 at 3.15.27 PM](/Users/asd123www/Downloads/Screen Shot 2021-12-05 at 3.15.27 PM.png) 

This picture contains all the hash situations described above: retransmission, previous segment lost, the breakup process, and the handshake process. As you can see the TCP worked well to handle these challenges.

The `perf_client` speed:

```
sending ...
receiving ...
0.60 KB/s
sending ...
receiving ...
0.64 KB/s
sending ...
receiving ...
0.62 KB/s
sending ...
receiving ...
0.66 KB/s
sending ...
receiving ...
0.64 KB/s
sending ...
receiving ...
0.54 KB/s
sending ...
receiving ...
0.73 KB/s
sending ...
receiving ...
0.73 KB/s
sending ...
receiving ...
0.49 KB/s
sending ...
receiving ...
0.65 KB/s
```



### Checkpoint 9 (CP9)

`client`:

```
ip address: 10.100.3.2
loop #1 ok.
loop #2 ok.
loop #3 ok.
```

`server`:

```
Bad file descriptor
new connection
6 12 13 14 63 68 70 72 74 76 78 80 82 84 86 87 88 89 1549 3009 4184 5644 7104 8279 9739 11199 12374 13834 15000 all: 15000
new connection
6 12 13 14 63 68 70 72 74 76 78 80 82 84 86 87 88 89 4184 7104 8279 12374 15000 all: 15000
new connection
6 12 13 14 63 68 70 72 74 76 78 80 82 84 86 87 88 89 1549 3009 4184 5644 7104 8279 9739 11199 12374 13834 15000 all: 15000
```

I think the `Bad file descriptor` for `mySocketNumberOffset`, I just want to distinguish real socket numbers from mine.

### Checkpoint 10 (CP10).

`server`:

```
Bad file descriptor
new connection
all: 1460000
```

`client`:

```
sending ...
receiving ...
3.43 KB/s
sending ...
receiving ...
3.42 KB/s
sending ...
receiving ...
3.42 KB/s
sending ...
receiving ...
3.44 KB/s
sending ...
receiving ...
3.28 KB/s
sending ...
receiving ...
3.52 KB/s
sending ...
receiving ...
3.19 KB/s
sending ...
receiving ...
3.85 KB/s
sending ...
receiving ...
3.63 KB/s
sending ...
receiving ...
3.43 KB/s
```

