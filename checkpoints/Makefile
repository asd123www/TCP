
all: tcp testTCPSender testTCPReceiver echo_client echo_server networkStack perf_client perf_server

src = $(wildcard ./*.c)  
obj = $(patsubst %.c, %.o, $(src))  
#obj = $(src:%.c=%.o)  

target = tcp testTCPSender testTCPReceiver

tcp: test.o socket.o tcp.o device.o packetio.o route.o ip.o ringbuffer.o
	gcc test.o socket.o tcp.o ringbuffer.o ip.o packetio.o route.o device.o -o tcp -lpcap -lpthread

testTCPSender: testTCPSender.o socket.o tcp.o device.o packetio.o route.o ip.o ringbuffer.o
	gcc testTCPSender.o socket.o tcp.o ringbuffer.o ip.o packetio.o route.o device.o -o testTCPSender -lpcap -lpthread

testTCPReceiver: testTCPReceiver.o socket.o tcp.o device.o packetio.o route.o ip.o ringbuffer.o
	gcc testTCPReceiver.o socket.o tcp.o ringbuffer.o ip.o packetio.o route.o device.o -o testTCPReceiver -lpcap -lpthread


networkStack: networkStack.o socket.o tcp.o device.o packetio.o route.o ip.o ringbuffer.o
	gcc networkStack.o socket.o tcp.o ringbuffer.o ip.o packetio.o route.o device.o -o networkStack -lpcap -lpthread

perf_client: perf_client.o unp.o socket.o tcp.o device.o packetio.o route.o ip.o ringbuffer.o
	gcc perf_client.o unp.o socket.o tcp.o ringbuffer.o ip.o packetio.o route.o device.o -lpcap -lpthread -Wl,--wrap=socket,--wrap=bind,--wrap=listen,--wrap=connect,--wrap=accept,--wrap=read,--wrap=write,--wrap=close -o perf_client
perf_server: perf_server.o unp.o socket.o tcp.o device.o packetio.o route.o ip.o ringbuffer.o
	gcc perf_server.o unp.o socket.o tcp.o ringbuffer.o ip.o packetio.o route.o device.o -lpcap -lpthread -Wl,--wrap=socket,--wrap=bind,--wrap=listen,--wrap=connect,--wrap=accept,--wrap=read,--wrap=write,--wrap=close -o perf_server 

echo_client: echo_client.o unp.o socket.o tcp.o device.o packetio.o route.o ip.o ringbuffer.o
	gcc echo_client.o unp.o socket.o tcp.o ringbuffer.o ip.o packetio.o route.o device.o -lpcap -lpthread -Wl,--wrap=socket,--wrap=bind,--wrap=listen,--wrap=connect,--wrap=accept,--wrap=read,--wrap=write,--wrap=close -o echo_client
echo_server: echo_server.o unp.o socket.o tcp.o device.o packetio.o route.o ip.o ringbuffer.o
	gcc echo_server.o unp.o socket.o tcp.o ringbuffer.o ip.o packetio.o route.o device.o -lpcap -lpthread -Wl,--wrap=socket,--wrap=bind,--wrap=listen,--wrap=connect,--wrap=accept,--wrap=read,--wrap=write,--wrap=close -o echo_server 


# echo_client: echo_client.o unp.o
# 	gcc echo_client.o unp.o -o echo_client
# echo_server: echo_server.o unp.o
# 	gcc echo_server.o unp.o -o echo_server

# for test
perf_client.o:
	gcc -c perf_client.c -o perf_client.o

perf_server.o:
	gcc -c perf_server.c -o perf_server.o

echo_client.o:
	gcc -c echo_client.c -o echo_client.o

echo_server.o:
	gcc -c echo_server.c -o echo_server.o

unp.o:
	gcc -c unp.c -o unp.o


# lower layer files.
networkStack.o:
	gcc -c networkStack.c -o networkStack.o

testTCPSender.o:
	gcc -c testTCPSender.c -o testTCPSender.o

testTCPReceiver.o:
	gcc -c testTCPReceiver.c -o testTCPReceiver.o

test.o: test.c
	gcc -c test.c -o test.o

tcp.o: tcp.c
	gcc -c tcp.c -o tcp.o

socket.o: socket.c
	gcc -c socket.c -o socket.o

ringbuffer.o: ringbuffer.c
	gcc -c ringbuffer.c -o ringbuffer.o

device.o: device.c
	gcc -c device.c -o device.o

packetio.o: packetio.c
	gcc -c packetio.c -o packetio.o

route.o: route.c
	gcc -c route.c -o route.o  

ip.o: ip.c
	gcc -c ip.c -o ip.o


.PHONY: clean
clean:
	rm -rf $(obj) $(target)
	clear

run:
	sudo ./tcp


run-testTCPTX:
	sudo ./testTCPSender


run-testTCPRX:
	sudo ./testTCPReceiver

codeinfo:
	git ls-files | xargs wc -l

test-echo_server: 
	sudo ./echo_server

test-echo_client:
	sudo ./echo_client 10.100.1.2

test-perf_server: 
	sudo ./perf_server

test-perf_client:
	sudo ./perf_client 10.100.1.2