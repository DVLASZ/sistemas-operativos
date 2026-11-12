/**
 * @file
 * @brief Informa tamano, tipo, permisos, enlaces y ultima modificacion de un archivo
 * @author Edward Esteban Dávila Salazar
 * @author Miguel Angel Perez Mera
 */

#include <stdio.h>
#include <sys/stat.h>

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
	return 0;
}
