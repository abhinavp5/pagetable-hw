CC = gcc
CFLAGS = -Wall
LDFLAGS = -Wall


all: libmlpt.a

libmlpt.a: mlpt.o
	ar rcs libmlpt.a mlpt.o

mlpt.o: main.c mlpt.h config.h
	$(CC) $(CFLAGS) -c main.c -o mlpt.o

.PHONY: clean
clean:
	rm -f *.o libmlpt.a
