CC = gcc
CFLAGS = -Wall -g -I. -DDEBUG
DEPS =

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

PROGS = p1 p2 p3

all: $(PROGS)

p1: p1.o stub.o
	$(CC) -o $@ $^ $(CFLAGS)

p2: p2.o stub.o
	$(CC) -o $@ $^ $(CFLAGS)

p3: p3.o stub.o
	$(CC) -o $@ $^ $(CFLAGS)


stub.o: stub.c

clean:
	rm -f *.o $(PROGS)
