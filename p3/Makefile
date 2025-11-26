CC = gcc
CFLAGS = -Wall -g -I. -DDEBUG
LDFLAGS = -lpthread

# Regla genérica para compilar .c a .o
%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

# Programa final
all: client server

client: client.o stub.o
	$(CC) -o client client.o stub.o $(LDFLAGS)
	
server: server.o stub.o
	$(CC) -o server server.o stub.o $(LDFLAGS)

clean:
	rm -f *.o client server
