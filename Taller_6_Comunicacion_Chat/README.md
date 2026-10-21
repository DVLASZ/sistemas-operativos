# Taller: chat con sockets TCP e hilos POSIX

**Autores:** Edward Esteban Dávila Salazar, Miguel Angel Perez Mera ([@miguelperez80](https://github.com/miguelperez80))

Laboratorio de Sistemas Operativos. Servidor y cliente de chat sobre
sockets TCP IPv4 que usan hilos POSIX:

- el **servidor** crea un hilo por cada cliente conectado y difunde cada
  mensaje a los demás; la lista de clientes (descriptores y apodos) que
  comparten los hilos se protege con un mutex;
- el **cliente** usa un hilo receptor (muestra lo que llega del servidor)
  y el hilo principal como emisor (lee el teclado con `select()` y envía).

Además del código de la guía, el taller incluye:

- **Apodos** (ejercicio propuesto): lo primero que escribe el cliente es su
  apodo; los mensajes se difunden como `[alice] Hola.`.
- **Dirección del cliente** en la consola del servidor (`ip:puerto`) al
  entrar, al escribir y al salir.
- **Corrección de `SIGPIPE`:** los `send()` usan `MSG_NOSIGNAL`, de modo que
  un cliente que se va de golpe no mata al servidor.

## Contenido

- `src/chat.h` — constantes compartidas: `PORT`, `BUFSIZE`, `MSGSIZE`,
  `NICK_SIZE`.
- `src/chat_server.h`, `src/chat_server.c` — servidor de chat.
- `src/chat_client.h`, `src/chat_client.c` — cliente de chat.
- `Makefile` — construye `chat_server` y `chat_client` (`make`), limpia lo
  generado (`make clean`) y genera la documentación (`make doc`). No fue
  publicado con la guía; se escribió con las mismas banderas que ella
  indica (`gcc -std=c11 -Wall -Wextra -pthread`).
- `Doxyfile` — configuración de Doxygen para documentar `src/`.
- `bitacora.md` — compilación, ejecuciones, respuestas a los puntos para
  discutir en clase, desarrollo del ejercicio de los apodos y observaciones
  sobre el código base.

## Compilar y ejecutar

```bash
make
```

Se necesitan al menos tres terminales:

```bash
./chat_server                # terminal 1: servidor (puerto 8080)
./chat_client                # terminal 2: cliente A
./chat_client                # terminal 3: cliente B
./chat_client 192.168.1.10   # opcional: cliente que apunta a otra máquina
```

Cada cliente pide primero su apodo (si se pulsa Enter sin escribirlo, el
servidor le asigna uno, `cliente<número>`) y después los mensajes, que se
envían con Enter. Ctrl-D cierra un cliente. `make clean` borra los
ejecutables.

## Generar la documentación

```bash
doxygen Doxyfile      # o: make doc
```

Genera `docs/html/index.html`. No se versiona (ver `.gitignore` de la
raíz del repositorio): se regenera cuando se necesita.

## Requisitos

- `gcc` con soporte de C11 y `make`
- `doxygen` (solo para generar la documentación)
