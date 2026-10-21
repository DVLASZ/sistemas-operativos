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
    int              server_fd, client_fd;   // Sockets: el de escucha y el de cada cliente aceptado.
    struct sockaddr_in addr, client_addr;    // Dirección del servidor y la del cliente que se conecta.
    socklen_t        client_len;             // Tamaño de la dirección del cliente (lo rellena accept).
    client_conn_t   *conn;                   // Datos de la conexión que se le entregan al hilo.
    pthread_t        tid;                    // Identificador del hilo de cada cliente.
    int              opt = 1;                // Valor activado para SO_REUSEADDR.

    // Dejar la lista de clientes vacía y el mutex listo antes de aceptar a nadie.
    client_list_init();

    // Crear el socket de escucha y asociarlo al puerto del chat.
    server_fd = socket(AF_INET, SOCK_STREAM, 0);   // TCP sobre IPv4.
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Permite reiniciar el servidor sin esperar a que se libere el puerto.
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Rellenar la dirección: cualquier interfaz de red, en el puerto del chat.
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(PORT);   // htons: pasa el puerto al orden de bytes de la red.

    // Asociar el socket a esa dirección y puerto.
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Marcar el socket como pasivo: queda esperando conexiones (hasta 10 en cola).
    if (listen(server_fd, 10) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Servidor de chat en el puerto %d\n", PORT);

    // Aceptar conexiones y entregar cada una a un hilo independiente.
    while (1) {
        client_len = sizeof(client_addr);
        // accept bloquea hasta que llega un cliente y devuelve un socket nuevo solo para él.
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
                           &client_len);
        if (client_fd == -1) {
            perror("accept");
            continue;   // Un fallo al aceptar no debe tumbar el servidor: se sigue esperando.
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

        // Registrar al cliente en la lista para que reciba los mensajes de los demás.
        client_list_add(client_fd);

        // Crear el hilo que atenderá a este cliente; si falla, se deshace todo lo anterior.
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

    // El ciclo anterior no termina; estas líneas solo se alcanzarían si se agregara una salida.
    close(server_fd);
    return 0;
}

void client_list_init(void) {
    int i;
    // -1 marca una posición libre (un descriptor válido nunca es negativo).
    for (i = 0; i < MAX_CLIENTS; i++) {
        clients.fds[i] = -1;
        clients.nicknames[i][0] = '\0';
    }
    pthread_mutex_init(&clients.lock, NULL);
}

void client_list_add(int fd) {
    int i;
    // Sección crítica: un solo hilo a la vez modifica la lista.
    pthread_mutex_lock(&clients.lock);
    // Guardar el descriptor en la primera posición libre.
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients.fds[i] == -1) {
            clients.fds[i] = fd;
            clients.nicknames[i][0] = '\0';   // Aún sin apodo.
            break;
        }
    }
    pthread_mutex_unlock(&clients.lock);
}

void client_list_remove(int fd) {
    int i;
    pthread_mutex_lock(&clients.lock);
    // Buscar el descriptor y dejar su posición libre otra vez, con su apodo borrado.
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients.fds[i] == fd) {
            clients.fds[i] = -1;
            clients.nicknames[i][0] = '\0';
            break;
        }
    }
    pthread_mutex_unlock(&clients.lock);
}

void client_list_set_nickname(int fd, const char *nickname) {
    int i;
    // El apodo se escribe bajo el mutex: otros hilos pueden leerlo al mismo tiempo.
    pthread_mutex_lock(&clients.lock);
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients.fds[i] == fd) {
            // strncpy no pone el terminador si el texto no cabe: se agrega a mano.
            strncpy(clients.nicknames[i], nickname, NICK_SIZE - 1);
            clients.nicknames[i][NICK_SIZE - 1] = '\0';
            break;
        }
    }
    pthread_mutex_unlock(&clients.lock);
}

void client_list_get_nickname(int fd, char *dest) {
    int i;
    dest[0] = '\0';   // Si el descriptor no está registrado, el resultado es una cadena vacía.
    pthread_mutex_lock(&clients.lock);
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients.fds[i] == fd) {
            strncpy(dest, clients.nicknames[i], NICK_SIZE - 1);
            dest[NICK_SIZE - 1] = '\0';
            break;
        }
    }
    pthread_mutex_unlock(&clients.lock);
}

int read_line(int fd, char *dest, int size) {
    int     total = 0;   // Caracteres guardados hasta ahora.
    char    c;           // Byte que se acaba de leer.
    ssize_t n;           // Resultado de recv.

    // Un byte por vez, para no consumir nada más allá del salto de línea.
    while ((n = recv(fd, &c, 1, 0)) > 0) {
        if (c == '\n') {
            dest[total] = '\0';   // Fin de la línea: cerrar la cadena y devolverla.
            return total;
        }
        // El retorno de carro (\r) se ignora; lo que no cabe en el búfer se descarta.
        if (c != '\r' && total < size - 1) {
            dest[total++] = c;
        }
    }
    return -1;   // El cliente cerró la conexión o hubo un error.
}

void broadcast(int sender_fd, const char *msg, int len) {
    int i, count = 0;
    int snapshot[MAX_CLIENTS];   // Copia local de los destinatarios.

    // Copiar los destinatarios mientras la lista está protegida.
    pthread_mutex_lock(&clients.lock);
    for (i = 0; i < MAX_CLIENTS; i++) {
        // Se salta las posiciones libres y al propio emisor (no recibe su mensaje).
        if (clients.fds[i] != -1 && clients.fds[i] != sender_fd) {
            snapshot[count++] = clients.fds[i];
        }
    }
    pthread_mutex_unlock(&clients.lock);

    // Enviar fuera de la sección crítica para no retener el mutex.
    // (send puede bloquearse si el cliente no lee; así no frena a los demás hilos).
    // MSG_NOSIGNAL: si un cliente ya se fue, send falla con error en lugar de matar
    // al servidor con SIGPIPE.
    for (i = 0; i < count; i++) {
        send(snapshot[i], msg, len, MSG_NOSIGNAL);
    }
}

void *client_thread(void *arg) {
    client_conn_t *conn = (client_conn_t *)arg;   // Datos que reservó main en el heap.
    int    fd = conn->fd;                         // Socket de este cliente.
    char   peer[INET_ADDRSTRLEN];                 // Su dirección IP en texto.
    char   origin[INET_ADDRSTRLEN + 8];           // Dirección y puerto del cliente: "ip:puerto".
    char   nick[NICK_SIZE];                       // Su apodo.
    char   buf[BUFSIZE];                          // Lo que se recibe del cliente.
    char   msg[MSGSIZE];                          // El mensaje con prefijo que se difunde.
    ssize_t n;                                    // Bytes recibidos en cada recv.

    // Convertir la IP binaria del cliente a texto, y armar "ip:puerto" para la consola.
    inet_ntop(AF_INET, &conn->addr.sin_addr, peer, INET_ADDRSTRLEN);
    snprintf(origin, sizeof(origin), "%s:%d", peer, ntohs(conn->addr.sin_port));
    // Los datos pertenecen al hilo; ya se copiaron los campos necesarios.
    free(conn);

    // El primer mensaje del cliente es su apodo.
    if (read_line(fd, nick, NICK_SIZE) < 0) {
        client_list_remove(fd);   // Se fue sin presentarse: no hay nada que anunciar.
        close(fd);
        return NULL;
    }
    if (nick[0] == '\0') {
        snprintf(nick, NICK_SIZE, "cliente%d", fd);   // Apodo vacío: se le asigna uno.
    }
    // Guardarlo junto al descriptor, en la lista compartida (bajo el mutex).
    client_list_set_nickname(fd, nick);

    // Avisar a los demás clientes (con la IP) y en la consola del servidor (con ip:puerto).
    snprintf(msg, MSGSIZE,
             "[servidor] %s (%s) se unió al chat\n", nick, peer);
    broadcast(fd, msg, (int)strlen(msg));
    printf("[servidor] %s (%s) se unió al chat\n", nick, origin);
    fflush(stdout);

    // Recibir los mensajes del cliente y difundirlos a los demás hasta que se vaya.
    // recv devuelve 0 si el cliente cerró la conexión y -1 si hubo un error.
    while ((n = recv(fd, buf, BUFSIZE - 1, 0)) > 0) {
        buf[n] = '\0';   // Cerrar la cadena: recv no agrega el terminador.
        client_list_get_nickname(fd, nick);   // Se lee de la lista, que es la fuente del apodo.
        // La consola del servidor muestra quién es y desde qué dirección escribe.
        printf("[%s %s] %s", nick, origin, buf);
        fflush(stdout);
        // Los demás clientes ven solo el apodo.
        snprintf(msg, MSGSIZE, "[%s] %s", nick, buf);
        broadcast(fd, msg, (int)strlen(msg));
    }

    // El cliente se fue: avisar a los demás y en la consola.
    snprintf(msg, MSGSIZE,
             "[servidor] %s (%s) salió del chat\n", nick, peer);
    broadcast(fd, msg, (int)strlen(msg));
    printf("[servidor] %s (%s) salió del chat\n", nick, origin);
    fflush(stdout);

    // Quitarlo de la lista antes de cerrar su socket, y terminar el hilo.
    client_list_remove(fd);
    close(fd);
    return NULL;
}
