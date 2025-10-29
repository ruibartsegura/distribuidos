CC = gcc
CFLAGS = -Wall -g -I. -DDEBUG
DEPS =

# Regla genérica para compilar .c a .o
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

# Compila todos los programas
all: p1 p2 p3 stub

p1: p1.o
	$(CC) -o p1 p1.o $(CFLAGS)

p2: p2.o
	$(CC) -o p2 p2.o $(CFLAGS)

p3: p3.o
	$(CC) -o p3 p3.o $(CFLAGS)

stub: stub.o
	$(CC) -o stub stub.o $(CFLAGS)

clean:
	rm -f *.o p1 p2 p3 stub

