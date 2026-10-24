#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/limits.h>


/** @brief Puerto por defecto */
#define DEFAULT_PORT 1234 

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
	int size; /**< Tamano del archivo a transferir, -1 si hay error ej. no existe*/
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
*
* @return 1 en caso de exito, 0 si ocurre un error
*/
int receive_file(int s, char * destination_folder);

/**
* @brief Envia un archivo a un socket
*
* @param s Socket a enviar el archivo
* @param path Ruta al archivo a enviar
*
* @return 
*/
int send_file(int s, char * path);

#endif
