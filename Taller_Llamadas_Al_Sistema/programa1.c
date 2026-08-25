/**
 * @file programa1.c
 * @brief Identifica un proceso y su padre, y escribe un mensaje en la
 *        salida estandar usando la llamada al sistema write(), no printf().
 *
 * Requisitos del taller:
 *  - Obtener pid propio (getpid) y pid del padre (getppid).
 *  - Escribir el mensaje en stdout usando write(2), no printf(3).
 *  - Verificar el valor de retorno de cada llamada al sistema.
 *  - Terminar con un codigo de salida distinto de cero.
 *
 * Documentacion consultada:
 *   man 2 getpid
 *   man 2 write
 *   man 3 perror
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * @brief Codigo de salida elegido por el autor para indicar terminacion
 *        normal de este programa (distinto de 0, que el sistema reserva
 *        para "exito").
 */
#define CODIGO_SALIDA 7

/**
 * @brief Escribe un buffer completo en un descriptor de archivo con write().
 *
 * write() puede escribir menos bytes de los pedidos (por ejemplo si la
 * escritura es interrumpida por una senal). Por eso se repite la llamada
 * hasta escribir todo el buffer o hasta que ocurra un error real.
 *
 * @param fd  Descriptor de archivo destino.
 * @param buf Buffer con los datos a escribir.
 * @param n   Cantidad de bytes a escribir.
 * @return 0 si se escribio todo el buffer; -1 si write() fallo (se informa
 *         la causa con perror antes de retornar).
 */
static int escribir_todo(int fd, const char *buf, size_t n) {
    size_t total_escrito = 0;

    while (total_escrito < n) {
        ssize_t escritos = write(fd, buf + total_escrito, n - total_escrito);

        if (escritos == -1) {
            if (errno == EINTR) {
                continue; /* llamada interrumpida por una senal: reintentar */
            }
            perror("write");
            return -1;
        }

        total_escrito += (size_t)escritos;
    }

    return 0;
}

/**
 * @brief Obtiene pid/ppid, arma el mensaje y lo escribe en la salida
 *        estandar mediante write(), verificando cada retorno.
 * @return No retorna de forma normal: el proceso termina con exit(),
 *         usando EXIT_FAILURE ante error o CODIGO_SALIDA en caso de exito.
 */
int main(void) {
    pid_t pid = getpid();
    pid_t ppid = getppid();

    char mensaje[128];
    int longitud = snprintf(mensaje, sizeof(mensaje),
                             "Proceso pid=%ld, proceso padre ppid=%ld\n",
                             (long)pid, (long)ppid);

    if (longitud < 0 || (size_t)longitud >= sizeof(mensaje)) {
        fprintf(stderr, "Error al construir el mensaje\n");
        exit(EXIT_FAILURE);
    }

    if (escribir_todo(STDOUT_FILENO, mensaje, (size_t)longitud) == -1) {
        exit(EXIT_FAILURE);
    }

    exit(CODIGO_SALIDA);
}
