CC=gcc
CFLAGS=-std=c99

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

gruff: gruff.o
	$(CC) -o $@ $^

.PHONY: clean test install

install:
	cp gruff /usr/local/bin/gruff

test: gruff
	./gruff a.bmp

clean:
	rm -f *.o gruff
