

tcp: test.o ip.o
    gcc main.o ip.o -o tcp  


main.o: main.c  
    gcc -c main.c -o main.o  

fun1.o: fun1.c  
    gcc -c fun1.c -o fun1.o  

fun2.o: fun2.c  
    gcc -c fun2.c -o fun2.o  
