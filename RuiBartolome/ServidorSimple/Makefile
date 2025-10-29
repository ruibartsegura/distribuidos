CC = gcc
CFLAGS = -Wall -g -I.
DEPS =

# Regla genérica para compilar .c a .o
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

# Compila ambos programas
all: client server

client: client.o
	$(CC) -o client client.o $(CFLAGS)

server: server.o
	$(CC) -o server server.o $(CFLAGS)

clean:
	rm -f *.o client server

