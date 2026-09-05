# Taller: creacion de procesos.
# Construye procesos a partir de procesos.c.

all: procesos

procesos: procesos.o
	gcc -o procesos procesos.o

procesos.o: procesos.c
	gcc -Wall -c -o procesos.o procesos.c

clean:
	rm -f *.o procesos

.PHONY: all clean
