/**
 * @file
 * @brief Utilidades de sistema de archivos propias del proyecto
 * @author Edward Esteban Dávila Salazar
 * @author Miguel Angel Perez Mera
 * @copyright MIT License
*/

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "fsutil.h"

int ensure_dir(const char * path) {
	struct stat st;

	//Si ya existe, debe ser un directorio
	if (stat(path, &st) == 0) {
		return S_ISDIR(st.st_mode);
	}

	//Crearlo; si otro proceso o hilo lo creo entre tanto, tambien es valido
	if (mkdir(path, 0755) == 0 || errno == EEXIST) {
		return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
	}

	return 0;
}

int program_dir(char * dest, size_t size) {
	char * slash;
	ssize_t n;

	if (size < 2) {
		return 0;
	}

	//En Linux, /proc/self/exe es un enlace a la ruta del ejecutable
	n = readlink("/proc/self/exe", dest, size - 1);
	if (n <= 0 || (size_t)n >= size - 1) {
		return 0; //Error, o ruta posiblemente truncada
	}
	dest[n] = '\0';

	//Quitar el nombre del programa y conservar la carpeta
	slash = strrchr(dest, '/');
	if (slash == NULL) {
		return 0;
	}
	*slash = '\0';

	return 1;
}
