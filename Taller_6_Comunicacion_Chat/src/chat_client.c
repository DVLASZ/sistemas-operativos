/**
 * @file
 * @brief Cliente de chat TCP con un hilo receptor y un hilo emisor.
 * @author Erwin Meza Vega <emezav@unicauca.edu.co>
 * @author Edward Esteban Dávila Salazar
 * @author Miguel Angel Perez Mera
 * @copyright MIT License
 */

#include "chat_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static volatile int running = 1; /**< Indicador de ejecución del cliente. */
static int sock_fd; /**< Descriptor de la conexión con el servidor. */

int main(int argc, char *argv[]) {
    struct sockaddr_in addr;
    pthread_t          recv_tid;
    char               buf[BUFSIZE];
    fd_set             fds;
    struct timeval     tv;
    const char        *host = "127.0.0.1";
    int                r;

    if (argc > 1) {
        host = argv[1];
    }

    // Crear el socket TCP y conectarlo a la dirección del servidor.
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(PORT);

    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        fprintf(stderr, "Dirección inválida: %s\n", host);
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    printf("Conectado al chat en %s:%d\n", host, PORT);

    // El apodo es lo primero que se envía: la primera línea que se escribe.
    printf("Escribe tu apodo: ");
    fflush(stdout);
    if (fgets(buf, BUFSIZE, stdin) == NULL) {
        close(sock_fd);
        return 0;   // EOF antes de presentarse.
    }
    if (send(sock_fd, buf, strlen(buf), MSG_NOSIGNAL) <= 0) {
        perror("send");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    printf("Escribe mensajes (Ctrl-D para salir):\n");

    // Recibir en otro hilo mientras el principal atiende el teclado.
    if (pthread_create(&recv_tid, NULL, receiver_thread, NULL) != 0) {
        perror("pthread_create");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    pthread_detach(recv_tid);

    // Esperar datos en stdin con un timeout para consultar running.
    // shutdown() actúa sobre el socket; no despierta este select() sobre stdin.
    while (running) {
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        tv.tv_sec  = 1;
        tv.tv_usec = 0;

        r = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
        if (r == -1 || !running) {
            break;
        }
        if (r == 0) {
            continue; // Timeout: volver a verificar running.
        }

        if (fgets(buf, BUFSIZE, stdin) == NULL) {
            break; // EOF: Ctrl-D.
        }

        if (!running) {
            break;
        }

        // Enviar al servidor la línea escrita (sin SIGPIPE si el servidor ya cerró).
        if (send(sock_fd, buf, strlen(buf), MSG_NOSIGNAL) <= 0) {
            break;
        }
    }

    running = 0;
    close(sock_fd);
    return 0;
}

void *receiver_thread(void *arg) {
    char    buf[BUFSIZE];
    ssize_t n;

    (void)arg;

    // Mostrar lo que difunde el servidor mientras el cliente siga activo.
    while (running) {
        n = recv(sock_fd, buf, BUFSIZE - 1, 0);
        if (n <= 0) {
            if (running) {
                printf("\nConexión cerrada por el servidor.\n");
                fflush(stdout);
            }
            break;
        }
        buf[n] = '\0';
        printf("%s", buf);
        fflush(stdout);
    }

    // Avisar al emisor que debe terminar y deshabilitar el socket.
    running = 0;
    shutdown(sock_fd, SHUT_RDWR);
    return NULL;
}
