/* 
 * Net util library
 * Author Erwin Meza Vega <emezav@gmail.com> 
*/

#include "netutil.h"

struct sockaddr_in * any_address(unsigned short port) {

  struct sockaddr_in * addr;

  addr = malloc(sizeof(struct sockaddr_in));

  if (addr == NULL) {
    return NULL;
  }

  memset(addr, 0, sizeof(struct sockaddr_in));
  addr->sin_family = AF_INET;
  addr->sin_port = htons(port);

  //Asignar la dirección INADDR_ANY
  addr->sin_addr.s_addr = INADDR_ANY;
  //if (inet_aton("0.0.0.0", &addr->sin_addr) == 0) {
  //    free(addr);
  //    return NULL;
  //}

  return addr;

}

struct sockaddr_in * address_by_ip(char * ip, unsigned short port) {
  struct sockaddr_in * addr;

  addr = malloc(sizeof(struct sockaddr_in));

  if (addr == NULL) {
    return NULL;
  }

  memset(addr, 0, sizeof(struct sockaddr_in));
  addr->sin_family = AF_INET;
  addr->sin_port = htons(port);

  if (inet_aton(ip, &addr->sin_addr) == 0) {
    free(addr);
    return NULL;
  }
  return addr;
}

struct addrinfo * address_by_hostname(char * hostname, unsigned short port, int flags) {

  // Criterios buscar la direccion IPv4
  struct addrinfo hints;

  struct addrinfo * res;

  struct sockaddr_in * addr;

  memset(&hints, 0, sizeof(struct addrinfo));

  //Configurar los criterios para buscar una direccion IPv4
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = flags;

  if (getaddrinfo(hostname, 0, &hints, &res) != 0) {
    return NULL;
  }

  return res;
}

