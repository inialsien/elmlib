
all: 
	$(CC) elm327_if.c -o elm327_if.o -ggdb -O0 -c 
	$(CC) elm327_bridge.c -o elm327_bridge elm327_if.o -ggdb -O0

clean:
	-rm *.o
