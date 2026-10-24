#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "protocol.h"

int receive_file(int s, char * destination_folder) {

	//TODO (siguiente commit)
	(void)s;
	(void)destination_folder;
	//1. Leer la informacion del archivo del socket
	//   En la informacion enviada por el otro extremo se tiene el nombre del archivo
	//2. Revisar la existencia y permisos sobre el directorio donde se desea recibir el archivo
	//3. Leer el contenido del socket y guardarlo al archivo local (open-read-write-close)
	return 0;
}

int send_file(int s, char * path) {
	//TODO (siguiente commit)
	(void)s;
	(void)path;
	//1. Verificar si la ruta corresponde a un archivo regular valido
	//2. Enviar un mensaje con la informacion del archivo a transferir
	//   El otro extremo necesita conocer al menos el nombre del archivo y su tamaño
	//3. Enviar el contenido del archivo, exactamente la cantidad de bytes reportadas
	
	return 0;
}

ssize_t read_all(int fd, void * buf, size_t count) {
	size_t total = 0;
	char * ptr = (char *)buf;
	ssize_t n;

	while (total < count) {
		n = read(fd, ptr + total, count - total);
		if (n == 0) {
			break; //El otro extremo cerro la conexion
		}
		if (n < 0) {
			if (errno == EINTR) {
				continue; //Una senal interrumpio la lectura: reintentar
			}
			return -1;
		}
		total += n;
	}

	return total;
}

ssize_t write_all(int fd, const void * buf, size_t count) {
	size_t total = 0;
	const char * ptr = (const char *)buf;
	ssize_t n;

	while (total < count) {
		n = write(fd, ptr + total, count - total);
		if (n < 0) {
			if (errno == EINTR) {
				continue;
			}
			return -1;
		}
		total += n;
	}

	return total;
}
