# Taller: comunicacion con sockets TCP e hilos POSIX (chat).
#
# make          compila el servidor (chat_server) y el cliente (chat_client)
# make clean    elimina los ejecutables
# make doc      genera la documentacion con Doxygen en docs/html

CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra
LDFLAGS = -pthread

PROGRAMAS = chat_server chat_client

all: $(PROGRAMAS)

# Cada programa depende de su fuente y de las cabeceras compartidas.
chat_server: src/chat_server.c src/chat_server.h src/chat.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ src/chat_server.c

chat_client: src/chat_client.c src/chat_client.h src/chat.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ src/chat_client.c

clean:
	rm -f $(PROGRAMAS)

doc:
	doxygen Doxyfile

.PHONY: all clean doc
