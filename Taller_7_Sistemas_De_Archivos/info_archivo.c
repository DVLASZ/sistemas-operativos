/**
 * @file
 * @brief Informa tamano, tipo, permisos, enlaces y ultima modificacion de un archivo
 * @author Edward Esteban Dávila Salazar
 * @author Miguel Angel Perez Mera
 */

#include <stdio.h>
#include <sys/stat.h>

/**
 * @brief Devuelve el nombre del tipo de archivo segun st_mode
 *
 * @param mode Campo st_mode de struct stat
 * @return Texto con el tipo
 */
static const char *tipo(mode_t mode) {
	if (S_ISREG(mode)) return "archivo regular";
	if (S_ISDIR(mode)) return "directorio";
	if (S_ISLNK(mode)) return "enlace simbolico";
	if (S_ISCHR(mode)) return "dispositivo de caracteres";
	if (S_ISBLK(mode)) return "dispositivo de bloques";
	if (S_ISFIFO(mode)) return "fifo";
	if (S_ISSOCK(mode)) return "socket";
	return "desconocido";
}

int main(int argc, char *argv[]) {
	struct stat st;

	if (argc != 2) {
		fprintf(stderr, "uso: %s archivo\n", argv[0]);
		return 1;
	}

	//lstat para que un enlace simbolico informe sobre si mismo y no sobre su destino
	if (lstat(argv[1], &st) == -1) {
		perror(argv[1]);
		return 1;
	}

	printf("archivo: %s\n", argv[1]);
	printf("tamano: %lld bytes\n", (long long)st.st_size);
	printf("tipo: %s\n", tipo(st.st_mode));
	return 0;
}
