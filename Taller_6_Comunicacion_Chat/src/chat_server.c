/**
 * @file
 * @brief Servidor de chat TCP que atiende a cada cliente con un hilo POSIX.
 * @author Erwin Meza Vega <emezav@unicauca.edu.co>
 * @author Edward Esteban Dávila Salazar
 * @author Miguel Angel Perez Mera
 * @copyright MIT License
 */

#include "chat_server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static client_list_t clients; /**< Lista de conexiones compartida entre los hilos. */

int main(void) {
    int              server_fd, client_fd;
    struct sockaddr_in addr, client_addr;
    socklen_t        client_len;
    client_conn_t   *conn;
    pthread_t        tid;
    int              opt = 1;

    client_list_init();

    // Crear el socket de escucha y asociarlo al puerto del chat.
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Permite reiniciar el servidor sin esperar a que se libere el puerto.
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Servidor de chat en el puerto %d\n", PORT);

    // Aceptar conexiones y entregar cada una a un hilo independiente.
    while (1) {
        client_len = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
                           &client_len);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        // Los datos van en el heap: así el siguiente accept no los sobrescribe
        // antes de que el hilo los lea.
        conn = malloc(sizeof(client_conn_t));
        if (conn == NULL) {
            perror("malloc");
            close(client_fd);
            continue;
        }
        conn->fd   = client_fd;
        conn->addr = client_addr;

        client_list_add(client_fd);

        if (pthread_create(&tid, NULL, client_thread, conn) != 0) {
            perror("pthread_create");
            client_list_remove(client_fd);
            close(client_fd);
            free(conn);
            continue;
        }
        // Nadie espera al hilo: sus recursos se liberan solos al terminar.
        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}

void client_list_init(void) {
    int i;
    for (i = 0; i < MAX_CLIENTS; i++) {
        clients.fds[i] = -1;
    }
    pthread_mutex_init(&clients.lock, NULL);
}

void client_list_add(int fd) {
    int i;
    // Sección crítica: un solo hilo a la vez modifica la lista.
    pthread_mutex_lock(&clients.lock);
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients.fds[i] == -1) {
            clients.fds[i] = fd;
            break;
        }
    }
    pthread_mutex_unlock(&clients.lock);
}

void client_list_remove(int fd) {
    int i;
    pthread_mutex_lock(&clients.lock);
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients.fds[i] == fd) {
            clients.fds[i] = -1;
            break;
        }
    }
    pthread_mutex_unlock(&clients.lock);
}

void broadcast(int sender_fd, const char *msg, int len) {
    int i, count = 0;
    int snapshot[MAX_CLIENTS];

    // Copiar los destinatarios mientras la lista está protegida.
    pthread_mutex_lock(&clients.lock);
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients.fds[i] != -1 && clients.fds[i] != sender_fd) {
            snapshot[count++] = clients.fds[i];
        }
    }
    pthread_mutex_unlock(&clients.lock);

    // Enviar fuera de la sección crítica para no retener el mutex.
    for (i = 0; i < count; i++) {
        send(snapshot[i], msg, len, 0);
    }
}

void *client_thread(void *arg) {
    client_conn_t *conn = (client_conn_t *)arg;
    int    fd = conn->fd;
    char   peer[INET_ADDRSTRLEN];
    char   buf[BUFSIZE];
    char   msg[MSGSIZE];
    ssize_t n;

    inet_ntop(AF_INET, &conn->addr.sin_addr, peer, INET_ADDRSTRLEN);
    // Los datos pertenecen al hilo; ya se copiaron los campos necesarios.
    free(conn);

    snprintf(msg, MSGSIZE,
             "[servidor] Cliente %d (%s) se unió al chat\n", fd, peer);
    broadcast(fd, msg, (int)strlen(msg));
    printf("%s", msg);
    fflush(stdout);

    // Recibir los mensajes del cliente y difundirlos a los demás hasta que se vaya.
    while ((n = recv(fd, buf, BUFSIZE - 1, 0)) > 0) {
        buf[n] = '\0';
        printf("[%d] %s", fd, buf);
        fflush(stdout);
        snprintf(msg, MSGSIZE, "[%d] %s", fd, buf);
        broadcast(fd, msg, (int)strlen(msg));
    }

    snprintf(msg, MSGSIZE,
             "[servidor] Cliente %d (%s) salió del chat\n", fd, peer);
    broadcast(fd, msg, (int)strlen(msg));
    printf("%s", msg);
    fflush(stdout);

    client_list_remove(fd);
    close(fd);
    return NULL;
}
