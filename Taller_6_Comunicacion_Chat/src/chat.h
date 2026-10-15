/**
 * @file
 * @brief Constantes compartidas por el cliente y el servidor de chat TCP.
 * @author Erwin Meza Vega <emezav@unicauca.edu.co>
 * @author Edward Esteban Dávila Salazar
 * @author Miguel Angel Perez Mera
 * @copyright MIT License
 */

#ifndef CHAT_H
#define CHAT_H

#define PORT 8080              /**< Puerto TCP del servidor. */
#define BUFSIZE 512            /**< Capacidad del búfer de texto, incluido el terminador nulo. */
#define MSGSIZE (BUFSIZE + 64) /**< Capacidad para el texto y el prefijo del servidor. */

#endif // CHAT_H
