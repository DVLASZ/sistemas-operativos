/**
 * @file
 * @brief Tipos e interfaz de las funciones del servidor de chat TCP.
 * @author Erwin Meza Vega <emezav@unicauca.edu.co>
 * @author Edward Esteban Dávila Salazar
 * @author Miguel Angel Perez Mera
 * @copyright MIT License
 */

#ifndef CHAT_SERVER_H
#define CHAT_SERVER_H

#include <netinet/in.h>
#include <pthread.h>

#include "chat.h"

#define MAX_CLIENTS 32 /**< Capacidad de la lista de descriptores activos. */

/** @brief Datos de la conexión que el hilo principal entrega al hilo de cliente. */
typedef struct {
    int fd;                   /**< Descriptor del socket conectado. */
    struct sockaddr_in addr;  /**< Dirección IPv4 y puerto del cliente. */
} client_conn_t;

/** @brief Lista compartida de clientes, protegida mediante exclusión mutua. */
typedef struct {
    int fds[MAX_CLIENTS];  /**< Descriptores activos; -1 indica una entrada libre. */
    char nicknames[MAX_CLIENTS][NICK_SIZE]; /**< Apodo de cada cliente, en la misma posición que su descriptor. */
    pthread_mutex_t lock;  /**< Mutex que protege el acceso a la lista y a los apodos. */
} client_list_t;

/**
 * @brief Inicializa las entradas libres y el mutex de la lista de clientes.
 * @pre No hay hilos accediendo a la lista y el mutex no está inicializado.
 */
void client_list_init(void);

/**
 * @brief Registra un descriptor en la primera entrada libre, si existe.
 * @param fd Descriptor del socket conectado.
 * @pre La lista de clientes está inicializada.
 */
void client_list_add(int fd);

/**
 * @brief Libera la entrada asociada a un descriptor, si está registrado.
 * @param fd Descriptor que se desea eliminar de la lista.
 * @pre La lista de clientes está inicializada.
 */
void client_list_remove(int fd);

/**
 * @brief Guarda el apodo de un cliente junto a su descriptor, bajo el mutex.
 * @param fd Descriptor del cliente registrado.
 * @param nickname Apodo, terminado en nulo; se trunca a NICK_SIZE - 1 caracteres.
 * @pre La lista de clientes está inicializada.
 */
void client_list_set_nickname(int fd, const char *nickname);

/**
 * @brief Copia el apodo de un cliente, bajo el mutex.
 * @param fd Descriptor del cliente registrado.
 * @param dest Búfer de al menos NICK_SIZE bytes; queda vacío si el descriptor no está registrado.
 * @pre La lista de clientes está inicializada.
 */
void client_list_get_nickname(int fd, char *dest);

/**
 * @brief Lee una línea terminada en salto de línea, un byte por vez.
 *
 * Lee solo hasta el salto de línea, de modo que no consume nada del mensaje siguiente. Si la
 * línea es más larga que el búfer, guarda el principio y descarta el resto.
 *
 * @param fd Descriptor del socket.
 * @param dest Búfer que recibe la línea, sin el salto de línea ni el retorno de carro.
 * @param size Capacidad del búfer, incluido el terminador nulo.
 * @return Cantidad de caracteres guardados, o -1 si el cliente cerró la conexión o hubo un error.
 */
int read_line(int fd, char *dest, int size);

/**
 * @brief Difunde un mensaje a los clientes registrados, excepto al emisor.
 *
 * Copia los descriptores bajo el mutex y lo libera antes de enviar.
 * @param sender_fd Descriptor del emisor que se excluye de la difusión.
 * @param msg Mensaje que se enviará.
 * @param len Cantidad de bytes del mensaje, sin el terminador nulo.
 * @pre La lista está inicializada y msg contiene al menos len bytes.
 */
void broadcast(int sender_fd, const char *msg, int len);

/**
 * @brief Atiende una conexión: lee el apodo, difunde sus mensajes y cierra su socket al salir.
 * @param arg Apuntador a client_conn_t reservado con malloc(); el hilo lo libera.
 * @return NULL cuando termina la conexión.
 * @pre arg contiene una conexión válida cuyo descriptor ya fue registrado.
 */
void *client_thread(void *arg);

#endif // CHAT_SERVER_H
