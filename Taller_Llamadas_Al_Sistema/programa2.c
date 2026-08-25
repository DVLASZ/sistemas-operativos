/**
 * @file programa2.c
 * @brief Imprime el mismo texto dos veces -una con printf(), otra con
 *        write()-, en ese orden, sin fflush y sin salto de linea final.
 *
 * Objetivo: observar que el orden de aparicion en pantalla no coincide
 * con el orden en que se llaman las funciones en el codigo. La
 * explicacion se desarrolla en bitacora.md (punto 5).
 *
 * Documentacion consultada:
 *   man 3 printf
 *   man 2 write
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
                continue;
            }
            perror("write");
            return -1;
        }

        total_escrito += (size_t)escritos;
    }

    return 0;
}

/**
 * @brief Imprime el mismo texto con printf() y luego con write(), sin
 *        fflush ni salto de linea, para exponer la diferencia entre el
 *        buffer de biblioteca y la entrega directa al kernel.
 * @return EXIT_SUCCESS si ambas escrituras tuvieron exito; EXIT_FAILURE
 *         en caso de error.
 */
int main(void) {
    const char *texto = "Hola desde el taller de llamadas al sistema";

    /* 1) printf: funcion de biblioteca, el texto queda en el buffer de
     *    stdio y no se envia de inmediato a la terminal. */
    if (printf("%s", texto) < 0) {
        fprintf(stderr, "Error en printf\n");
        exit(EXIT_FAILURE);
    }

    /* 2) write: llamada al sistema, entrega el texto directamente al
     *    kernel sin pasar por el buffer de stdio. */
    if (escribir_todo(STDOUT_FILENO, texto, strlen(texto)) == -1) {
        exit(EXIT_FAILURE);
    }

    /* Sin fflush intencionalmente: el vaciado del buffer de printf
     * ocurre recien al terminar el proceso (ver bitacora.md). */
    return 0;
}
