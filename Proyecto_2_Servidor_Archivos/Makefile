# Proyecto 2: servidor de archivos.
#
# make          compila el servidor (server) y el cliente (client)
# make clean    elimina lo generado
# make doc      genera la documentacion con Doxygen en docs/html

CC      = gcc
CFLAGS  = -std=gnu11 -Wall -Wextra -g
LDFLAGS = -pthread

all: server client

server: server.o fsutil.o util.o netutil.o protocol.o
	$(CC) $(CFLAGS) -o server server.o fsutil.o util.o netutil.o protocol.o $(LDFLAGS)

client: client.o fsutil.o util.o netutil.o protocol.o split.o
	$(CC) $(CFLAGS) -o client client.o fsutil.o util.o netutil.o protocol.o split.o $(LDFLAGS)

# Cada objeto depende de su fuente y de las cabeceras que usa.
%.o: %.c protocol.h util.h netutil.h split.h fsutil.h
	$(CC) $(CFLAGS) -c -o $@ $<

# Las librerias del profesor (util, netutil, split) se compilan tal como vienen,
# sin tocarlas; sus advertencias originales se silencian solo en estos objetos.
util.o netutil.o split.o: CFLAGS += -w

doc:
	doxygen Doxyfile

clean:
	rm -f *.o server client

.PHONY: all clean doc
