
CC = gcc
CFLAGS = -Wall -pthread

all: Servidor Cliente

servidor: Servidor.c
	$(CC) $(CFLAGS) Servidor.c -o Servidor

cliente: Cliente.c
	$(CC) $(CFLAGS) Cliente.c -o Cliente

clean:
	rm -f Servidor Cliente
