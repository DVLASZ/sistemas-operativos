#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "protocol.h"

/* tamano del bloque con el que se copia el contenido */
#define BLOCK 65536

volatile sig_atomic_t protocol_abort = 0;

int valid_filename(const char * name) {
	//los nombres que empiezan por "." (incluidos "." y "..") quedan para uso interno
	if (name[0] == '\0' || name[0] == '.') {
		return 0;
	}
	return strchr(name, '/') == NULL;
}

int receive_file(int s, char * destination_folder, const char * store_as) {
	file_info info;
	char path[PATH_MAX];
	char temp[PATH_MAX];
	char buf[BLOCK];
	int64_t left;
	size_t n;
	int fd;

	//1. la informacion del archivo llega primero
	if (read_all(s, &info, sizeof(info)) != (ssize_t)sizeof(info)) {
		return 0;
	}
	info.filename[sizeof(info.filename) - 1] = '\0';

	//el otro extremo avisa que el archivo no existe: size -1 y no sigue nada
	if (info.size == -1) {
		return -1;
	}

	//2. el nombre y el tamano vienen del otro extremo, no se confia en ellos
	if (info.size < 0 || !valid_filename(info.filename)) {
		return 0;
	}

	//con store_as el que recibe elige el nombre final (por ejemplo con sufijo)
	if (store_as == NULL) {
		store_as = info.filename;
	}
	if (snprintf(path, sizeof(path), "%s/%s", destination_folder, store_as) >= (int)sizeof(path) ||
		snprintf(temp, sizeof(temp), "%s/.%s.part", destination_folder, store_as) >= (int)sizeof(temp)) {
		return 0;
	}

	//3. se copia a un archivo temporal y al final se renombra: nadie ve un archivo a medias
	fd = open(temp, O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW, 0644);
	if (fd == -1) {
		return 0;
	}

	for (left = info.size; left > 0; left -= n) {
		n = left < BLOCK ? (size_t)left : BLOCK;
		if (read_all(s, buf, n) != (ssize_t)n || write_all(fd, buf, n) != (ssize_t)n) {
			//el otro extremo se fue a medias, hubo una senal o el disco fallo
			close(fd);
			unlink(temp);
			return 0;
		}
	}

	if (close(fd) != 0 || rename(temp, path) != 0) {
		unlink(temp);
		return 0;
	}
	return 1;
}

int send_file(int s, char * path) {
	struct stat st;
	file_info info;
	char buf[BLOCK];
	char * copy;
	int64_t left;
	ssize_t n;
	int fd;

	//1. tiene que ser un archivo regular
	if (stat(path, &st) == -1 || !S_ISREG(st.st_mode)) {
		return 0;
	}
	fd = open(path, O_RDONLY);
	if (fd == -1) {
		return 0;
	}

	//2. informacion del archivo: nombre sin la ruta, tamano y modo
	copy = strdup(path);
	if (copy == NULL) {
		close(fd);
		return 0;
	}
	memset(&info, 0, sizeof(info));
	info.size = st.st_size;
	info.mode = st.st_mode;
	strncpy(info.filename, basename(copy), sizeof(info.filename) - 1);
	free(copy);

	if (write_all(s, &info, sizeof(info)) != (ssize_t)sizeof(info)) {
		close(fd);
		return 0;
	}

	//3. el contenido, exactamente size bytes. si el archivo cambia de tamano a mitad
	//   se corta, porque el otro extremo ya espera la cantidad anunciada
	for (left = info.size; left > 0; left -= n) {
		n = read(fd, buf, left < BLOCK ? (size_t)left : BLOCK);
		if (n <= 0 || write_all(s, buf, n) != n) {
			close(fd);
			return 0;
		}
	}

	close(fd);
	return 1;
}

int send_not_found(int s, const char * name) {
	file_info info;

	memset(&info, 0, sizeof(info));
	info.size = -1;
	strncpy(info.filename, name, sizeof(info.filename) - 1);

	return write_all(s, &info, sizeof(info)) == (ssize_t)sizeof(info);
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
			if (errno == EINTR && !protocol_abort) {
				continue; //Una senal sin importancia interrumpio la lectura: reintentar
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
			if (errno == EINTR && !protocol_abort) {
				continue;
			}
			return -1;
		}
		if (n == 0) {
			return -1; //no avanza: evitar un ciclo infinito
		}
		total += n;
	}

	return total;
}
