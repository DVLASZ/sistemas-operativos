/**
 * @file
 * @brief Interfaz del hilo receptor del cliente de chat TCP.
 * @author Erwin Meza Vega <emezav@unicauca.edu.co>
 * @author Edward Esteban Dávila Salazar
 * @author Miguel Angel Perez Mera
 * @copyright MIT License
 */

#ifndef CHAT_CLIENT_H
#define CHAT_CLIENT_H

#include "chat.h"

/**
 * @brief Recibe los mensajes del servidor y los muestra en la salida estándar.
 *
 * Al finalizar, marca running = 0 y deshabilita el socket con shutdown().
 * El emisor comprueba running al vencer el timeout de select() sobre stdin.
 *
 * @param arg Argumento requerido por pthread_create(); no se utiliza.
 * @return NULL al terminar la recepción.
 * @pre El socket del cliente está conectado al servidor.
 */
void *receiver_thread(void *arg);

#endif // CHAT_CLIENT_H
