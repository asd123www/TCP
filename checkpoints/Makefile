
all: tcp testTCPSender testTCPReceiver

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

testTCPSender.o:
	gcc -c testTCPSender.c -o testTCPSender.o

testTCPReceiver.o:
	gcc -c testTCPReceiver.c -o testTCPReceiver.o

test.o: test.c
	gcc -c test.c -o test.o



# lower layer files.

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