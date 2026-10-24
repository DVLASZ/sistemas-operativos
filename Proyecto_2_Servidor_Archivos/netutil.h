/* 
 * IPv4 util library
 * Author Erwin Meza Vega <emezav@gmail.com> 
*/

#ifndef NETUTIL_H
#define NETUTIL_H

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <resolv.h>



/**
 * @brief Crea una dirección IPv4 de tipo INADDR_ANY
 * @param port Puerto a atender
 * @return Apuntador a la dirección creada
 * @note Quien usa la función debe liberar la memoria reservada para la dirección
 */
struct sockaddr_in * any_address(unsigned short port);

/**
 * @brief Crea una dirección IPv4 por IP - puerto
 * @param ip Dirección IPv4 del servidor
 * @param port Puerto en el servidor
 * @return Apuntador a la dirección creada
 * @note Quien usa la función debe liberar la memoria reservada para la dirección
 */
struct sockaddr_in * address_by_ip(char * ip, unsigned short port);

/**
 * @brief Crea una dirección IPv4 por nombre de host - puerto
 * @param hostname Nombre de host del servidor
 * @param port Puerto en el servidor
 * @param flags Flags para pasar a getaddrinfo, ej. AI_NUMERICHOST, AI_PASSIVE, AI_NUMERICHOST
 * @return Lista de direcciones IPv4
 * @see getaddrinfo(2), freeaddrinfo(2)
 * @note Quien usa la función debe liberar la memoria reservada
 */
struct addrinfo * address_by_hostname(char * hostname, unsigned short port, int flags);

#endif
