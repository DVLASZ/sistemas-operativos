# Taller: procesos, scripts y llamadas al sistema.
# Construye infoproc a partir de infoproc.c.

all: infoproc

infoproc: infoproc.o
	gcc -o infoproc infoproc.o

infoproc.o: infoproc.c
	gcc -Wall -c -o infoproc.o infoproc.c

clean:
	rm -f *.o infoproc

.PHONY: all clean
