
all: tcp

src = $(wildcard ./*.c)  
obj = $(patsubst %.c, %.o, $(src))  
#obj = $(src:%.c=%.o)  

target = tcp

tcp: test.o device.o packetio.o route.o ip.o
	gcc test.o ip.o packetio.o route.o device.o -o tcp -lpcap -lpthread

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