
all: tcp

src = $(wildcard ./*.c)  
obj = $(patsubst %.c, %.o, $(src))  
#obj = $(src:%.c=%.o)  

target = tcp

tcp: test.o tcp.o device.o packetio.o route.o ip.o ringbuffer.o
	gcc test.o tcp.o ringbuffer.o ip.o packetio.o route.o device.o -o tcp -lpcap -lpthread

tcp.o: tcp.c
	gcc -c tcp.c -o tcp.o

ringbuffer.o: ringbuffer.c
	gcc -c ringbuffer.c -o ringbuffer.o

test.o: test.c
	gcc -c test.c -o test.o

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

run:
	sudo ./tcp