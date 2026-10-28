/**
 * @file
 * @brief Mensajes entre cliente y servidor y copia de archivos por el socket
 * @author Erwin Meza Vega <emezav@unicauca.edu.co>
 * @author Edward Esteban Dávila Salazar
 * @author Miguel Angel Perez Mera
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/limits.h>


/** @brief Puerto por defecto */
#define DEFAULT_PORT 1234

/** @brief Largo maximo del nombre de usuario */
#define USER_MAX 32

/**
* @brief Solicitud del cliente
*/
typedef struct {
	char operation[16]; /**< Operacion a realizar (get, put, exit)*/
	char filename[NAME_MAX]; /**< Nombre del archivo a transferir */
}request;

/**
* @brief Codigos de respuesta
*/
typedef enum {
	TRANSFER_AUTHORIZED,
	TRANSFER_DENIED,
} response_code;

/**
* @brief Respuesta del servidor para verificar archivos repetidos
*/
typedef struct {
	response_code code; /**< Codigo de respuesta */
	char message[256]; /**< Mensaje (opcional) */
}response;

/**
* @brief Informacion de un archivo a transferir
*/
typedef struct {
	int64_t size; /**< Tamano del archivo a transferir, -1 si hay error ej. no existe*/
	char filename[NAME_MAX]; /**< Nombre del archivo */
	mode_t mode; /**< Modo */
}file_info;



/**
* @brief Lee exactamente count bytes de un descriptor, repitiendo read() si llega menos
*
* Un read() sobre un socket puede devolver menos bytes de los pedidos; esta funcion
* sigue leyendo hasta completar la cantidad o hasta que el otro extremo cierre.
*
* @param fd Descriptor (socket o archivo)
* @param buf Buffer donde se guardan los bytes leidos
* @param count Cantidad de bytes que se deben leer
*
* @return Cantidad de bytes leidos (menor que count si el otro extremo cerro la
* conexion), o -1 si ocurre un error
*/
ssize_t read_all(int fd, void * buf, size_t count);

/**
* @brief Se pone en 1 desde un manejador de senal para que read_all y write_all dejen de
* reintentar cuando una senal los interrumpe
*/
extern volatile sig_atomic_t protocol_abort;

/**
* @brief Escribe exactamente count bytes en un descriptor, repitiendo write() si escribe menos
*
* @param fd Descriptor (socket o archivo)
* @param buf Bytes a escribir
* @param count Cantidad de bytes que se deben escribir
*
* @return Cantidad de bytes escritos (igual a count si todo salio bien), o -1 si
* ocurre un error
*/
ssize_t write_all(int fd, const void * buf, size_t count);

/**
* @brief Recibe un archivo de un socket y lo almacena en la carpeta especificada
*
* @param s Socket
* @param destination_folder Carpeta en la cual se almacenara el archivo
* @param store_as Nombre con el que se guarda, o NULL para usar el que manda el otro extremo
*
* @return 1 si recibio el archivo, 0 si ocurre un error, -1 si el otro extremo avisa que no existe
*/
int receive_file(int s, char * destination_folder, const char * store_as);

/**
* @brief Envia un archivo a un socket
*
* @param s Socket a enviar el archivo
* @param path Ruta al archivo a enviar
*
* @return 1 si se envio el archivo completo, 0 si ocurre un error
*/
int send_file(int s, char * path);

/**
* @brief Dice si un nombre de archivo se puede usar dentro de la carpeta de archivos
*
* Rechaza el nombre vacio, los que empiezan por "." (incluidos "." y ".."), y cualquier
* nombre con "/", para que un cliente
* no pueda escribir ni leer fuera de la carpeta.
*
* @param name Nombre recibido
*
* @return 1 si es valido, 0 si no
*/
int valid_filename(const char * name);

/**
* @brief Dice si un nombre de usuario es valido: de 1 a USER_MAX caracteres entre letras,
* numeros, "_", "-" y ".", sin empezar por "."
*
* @param name Nombre recibido
*
* @return 1 si es valido, 0 si no
*/
int valid_user(const char * name);

/**
* @brief Avisa que el archivo pedido no existe: manda un file_info con size -1 y nada mas
*
* @param s Socket
* @param name Nombre que se pidio
*
* @return 1 si se envio, 0 si ocurre un error
*/
int send_not_found(int s, const char * name);

#endif
