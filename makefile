CC = gcc
CFLAGS = -Wall
LDFLAGS = -Wall

.PHONY: all clean

all: libmlpt.a

libmlpt.a: mlpt.o
	ar rcs libmlpt.a mlpt.o

mlpt.o: mlpt.c mlpt.h config.h
	$(CC) $(CFLAGS) -c mlpt.c -o mlpt.o
	
clean:
	rm -f *.o libmlpt.a
