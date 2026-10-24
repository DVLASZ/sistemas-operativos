/**
 * @file
 * @brief Utilidades de sistema de archivos propias del proyecto
 * @author Edward Esteban Dávila Salazar
 * @author Miguel Angel Perez Mera
 * @copyright MIT License
*/

#ifndef FSUTIL_H
#define FSUTIL_H

#include <stddef.h>

/**
* @brief Crea un directorio si no existe
* @param path Ruta del directorio
* @return 1 si el directorio existe o fue creado, 0 si ocurre un error
*/
int ensure_dir(const char * path);

/**
* @brief Obtiene el directorio donde se encuentra el ejecutable en curso
* @param dest Buffer que recibe la ruta, sin la barra final
* @param size Capacidad del buffer
* @return 1 en caso de exito, 0 si ocurre un error o si la ruta no cabe en el buffer
*/
int program_dir(char * dest, size_t size);

#endif
