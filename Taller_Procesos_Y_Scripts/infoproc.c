/*
 * infoproc.c - informa Name, State, PPid y Threads de un proceso vivo,
 * leyendo /proc/PID/status exclusivamente con llamadas al sistema
 * (open, read, close), y reporta el pid/ppid del propio programa.
 *
 * Taller: procesos, scripts y llamadas al sistema.
 * Laboratorio de Sistemas Operativos.
 *
 * Autores: Edward Esteban Davila Salazar, Miguel Angel Perez Mera
 *
 * Uso: ./infoproc [pid]
 *   Sin argumento, informa sobre si mismo.
 *
 * Documentacion consultada:
 *   man 2 open
 *   man 2 read
 *   man 2 close
 *   man 5 proc
 *   man 2 getpid
 *   man 3 perror
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * Los archivos de /proc reportan tamaño 0 con stat, porque su contenido
 * se genera al leerlos: no hay ningun tamaño real que consultar de
 * antemano. Por eso el buffer se dimensiona con holgura en lugar de
 * confiar en el tamaño del archivo.
 */
#define TAM_BUFFER 65536

/*
 * Lee el archivo completo indicado por ruta hacia buffer (de capacidad
 * cap bytes), usando unicamente open/read/close, y lo termina en '\0'.
 *
 * read() entrega bytes sin atender lineas: puede devolver menos de lo
 * pedido y continuar en la llamada siguiente donde quedo, asi que se
 * repite hasta que devuelva 0 (fin de archivo) o el buffer se llene.
 *
 * @return Cantidad de bytes leidos (sin contar el '\0'); -1 si ocurrio
 *         un error (ya informado con perror) o si el contenido no cupo
 *         en el buffer (informado por este mismo mensaje de error).
 */
static ssize_t leer_archivo_completo(const char *ruta, char *buffer, size_t cap) {
    int fd = open(ruta, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return -1;
    }

    size_t total = 0;
    while (total < cap - 1) {
        ssize_t leidos = read(fd, buffer + total, cap - 1 - total);

        if (leidos == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("read");
            close(fd);
            return -1;
        }

        if (leidos == 0) {
            break;
        }

        total += (size_t)leidos;
    }

    /* Si se llego al limite del buffer sin encontrar el fin de archivo,
     * el contenido pudo quedar truncado: se confirma leyendo un byte
     * mas. Si todavia hay datos, el reporte se descarta en lugar de
     * entregarse incompleto. */
    if (total == cap - 1) {
        char sobrante;
        ssize_t leidos = read(fd, &sobrante, 1);
        if (leidos > 0) {
            fprintf(stderr, "infoproc: el contenido de %s no cabe en el buffer reservado\n", ruta);
            close(fd);
            return -1;
        }
    }

    if (close(fd) == -1) {
        perror("close");
        return -1;
    }

    buffer[total] = '\0';
    return (ssize_t)total;
}

/*
 * Busca en contenido (el texto completo de /proc/PID/status) la primera
 * linea que empieza por etiqueta (por ejemplo "Name:") y copia en valor
 * (de capacidad tam_valor) el texto que sigue, sin el salto de linea
 * final. Usa solo funciones de string.h sobre el buffer ya leido.
 *
 * @return 0 si encontro la etiqueta; -1 si no aparece en el contenido.
 */
static int extraer_campo(const char *contenido, const char *etiqueta, char *valor, size_t tam_valor) {
    size_t etiqueta_len = strlen(etiqueta);
    const char *linea = contenido;

    while (linea != NULL && *linea != '\0') {
        if (strncmp(linea, etiqueta, etiqueta_len) == 0) {
            const char *inicio_valor = linea + etiqueta_len;

            while (*inicio_valor == '\t' || *inicio_valor == ' ') {
                inicio_valor++;
            }

            const char *fin_linea = strchr(inicio_valor, '\n');
            size_t largo = (fin_linea != NULL) ? (size_t)(fin_linea - inicio_valor)
                                                : strlen(inicio_valor);

            if (largo >= tam_valor) {
                largo = tam_valor - 1;
            }

            memcpy(valor, inicio_valor, largo);
            valor[largo] = '\0';
            return 0;
        }

        linea = strchr(linea, '\n');
        if (linea != NULL) {
            linea++;
        }
    }

    return -1;
}

int main(int argc, char *argv[]) {
    pid_t pid_consultado;

    if (argc > 1) {
        char *fin;
        long valor = strtol(argv[1], &fin, 10);
        if (*fin != '\0' || valor <= 0) {
            fprintf(stderr, "infoproc: '%s' no es un pid valido\n", argv[1]);
            exit(EXIT_FAILURE);
        }
        pid_consultado = (pid_t)valor;
    } else {
        pid_consultado = getpid();
    }

    char ruta[64];
    int longitud = snprintf(ruta, sizeof(ruta), "/proc/%ld/status", (long)pid_consultado);
    if (longitud < 0 || (size_t)longitud >= sizeof(ruta)) {
        fprintf(stderr, "infoproc: no se pudo construir la ruta de /proc\n");
        exit(EXIT_FAILURE);
    }

    static char buffer[TAM_BUFFER];
    if (leer_archivo_completo(ruta, buffer, sizeof(buffer)) == -1) {
        exit(EXIT_FAILURE);
    }

    char nombre[256];
    char estado[256];
    char ppid_str[64];
    char hilos[64];

    if (extraer_campo(buffer, "Name:", nombre, sizeof(nombre)) == -1 ||
        extraer_campo(buffer, "State:", estado, sizeof(estado)) == -1 ||
        extraer_campo(buffer, "PPid:", ppid_str, sizeof(ppid_str)) == -1 ||
        extraer_campo(buffer, "Threads:", hilos, sizeof(hilos)) == -1) {
        fprintf(stderr, "infoproc: %s no tiene el formato esperado\n", ruta);
        exit(EXIT_FAILURE);
    }

    printf("PID %ld -> Name: %s | State: %s | PPid: %s | Threads: %s\n",
           (long)pid_consultado, nombre, estado, ppid_str, hilos);

    printf("Propio proceso: pid=%ld, ppid=%ld\n", (long)getpid(), (long)getppid());

    return EXIT_SUCCESS;
}
