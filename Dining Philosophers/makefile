CC=gcc
CFLAGS= -Wpedantic -std=gnu99

all: dine 

dine: dine.c
	$(CC) $(CFLAGS) -pthread dine.c -o dine 

clean:
	rm *.o
