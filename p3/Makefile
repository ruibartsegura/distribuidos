CC = gcc
CFLAGS = -Wall -g -I. -lpthread
DEPS =

# Regla genérica para compilar .c a .o
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

# Compila ambos programas
all: client

client: client.o
	$(CC) -o client client.o $(CFLAGS)

clean:
	rm -f *.o client server

