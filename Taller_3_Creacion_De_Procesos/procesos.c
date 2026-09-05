/**
 * @file procesos.c
 * @brief Taller: creación de procesos con fork, exec y waitpid.
 *
 * Crea dos procesos hijos que ejecutan comandos distintos, espera a
 * que ambos terminen y reporta el código de salida de cada uno y cuál
 * terminó primero.
 * @author Edward Esteban Davila Salazar
 * @author Miguel Angel Perez Mera
 *
 * Taller: creación de procesos.
 * Laboratorio de Sistemas Operativos.
 *
 * Documentación consultada:
 *   man 2 fork
 *   man 3 exec (execvp)
 *   man 2 waitpid
 *   man 2 wait
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/**
 * @brief Crea un proceso hijo que reemplaza su imagen con execvp().
 *
 * Distingue las tres ramas de fork(): error (-1), hijo (0) y padre (el
 * pid del hijo), cada una identificándose en la salida. El hijo llama
 * a execvp() con argv, cuyo último elemento debe ser (char *)NULL -no
 * el entero 0-, tal como exige la familia exec.
 *
 * Si execvp() tiene éxito, reemplaza por completo la imagen del
 * proceso (código, pila, datos): el código que sigue a la llamada
 * jamás se ejecuta, porque ese proceso ya no está corriendo este
 * programa. El fprintf()/_exit() de después de execvp() solo se
 * alcanzan si la llamada falló (por ejemplo, un comando inexistente):
 * en ese caso execvp() sí retorna, y el proceso sigue siendo este
 * mismo programa.
 *
 * @param etiqueta Nombre para identificar a este hijo en la salida.
 * @param argv     Argumentos del comando (argv[0] es el ejecutable),
 *                 terminados en (char *)NULL.
 * @return El pid del hijo en el proceso padre; -1 si fork() falló (ya
 *         informado por esta función).
 */
static pid_t lanzar_comando(const char *etiqueta, char *const argv[]) {
    pid_t pid = fork();

    if (pid == -1) {
        /* Rama de error: fork() no pudo crear el proceso. */
        fprintf(stderr, "[%s] fork fallo: %s\n", etiqueta, strerror(errno));
        return -1;
    }

    if (pid == 0) {
        /* Rama del hijo. */
        printf("[%s] hijo (pid %ld): voy a ejecutar '%s'\n",
               etiqueta, (long)getpid(), argv[0]);
        fflush(stdout);

        execvp(argv[0], argv);

        /* Solo se llega aca si execvp() fallo (ver comentario de la
         * funcion). Si el exec hubiera tenido exito, esta linea no
         * se ejecutaria nunca. */
        fprintf(stderr, "[%s] execvp de '%s' fallo: %s\n",
                etiqueta, argv[0], strerror(errno));
        _exit(EXIT_FAILURE);
    }

    /* Rama del padre. */
    printf("[%s] padre (pid %ld): cree al hijo %ld\n",
           etiqueta, (long)getpid(), (long)pid);

    return pid;
}

/**
 * @brief Crea dos hijos con comandos distintos, espera a ambos con
 *        waitpid() e informa el código de salida de cada uno y cuál
 *        terminó primero.
 * @return EXIT_SUCCESS si se pudieron crear y esperar los dos hijos;
 *         EXIT_FAILURE si fork() o waitpid() fallaron.
 */
int main(void) {
    /*
     * stdout solo es line-buffered por defecto cuando esta conectado a
     * una terminal interactiva; si se redirige a un archivo o a una
     * tuberia (por ejemplo al correr esto por SSH sin terminal), pasa
     * a ser fully-buffered. fork() duplica la memoria del proceso tal
     * como esta en ese instante, incluido lo que quedo en el buffer de
     * stdout sin escribirse todavia: si no se vacia antes de cada
     * fork(), cada hijo hereda una copia de ese buffer pendiente y lo
     * vuelve a imprimir cuando el suyo se vacia, duplicando la salida
     * del padre. Forzar aca el modo line-buffered evita ese problema
     * sin tener que llamar fflush() a mano en cada printf().
     */
    setvbuf(stdout, NULL, _IOLBF, 0);

    printf("Proceso principal, pid=%ld\n", (long)getpid());

    char *cmd1[] = {"sleep", "2", (char *)NULL};
    char *cmd2[] = {"echo", "Hola desde el segundo hijo", (char *)NULL};

    pid_t pid1 = lanzar_comando("hijo1", cmd1);
    pid_t pid2 = lanzar_comando("hijo2", cmd2);

    if (pid1 == -1 || pid2 == -1) {
        fprintf(stderr, "No se pudieron crear los dos procesos hijos\n");
        exit(EXIT_FAILURE);
    }

    int hijos_pendientes = 2;
    int primero_informado = 0;

    while (hijos_pendientes > 0) {
        int status;
        /* pid=-1 espera a CUALQUIER hijo: el que devuelve primero es
         * el que realmente termino primero, sin importar el orden en
         * que se crearon. */
        pid_t terminado = waitpid(-1, &status, 0);

        if (terminado == -1) {
            fprintf(stderr, "waitpid fallo: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        const char *etiqueta = (terminado == pid1) ? "hijo1" : "hijo2";

        if (WIFEXITED(status)) {
            printf("[%s] (pid %ld) termino con codigo de salida %d",
                   etiqueta, (long)terminado, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("[%s] (pid %ld) termino por la senal %d",
                   etiqueta, (long)terminado, WTERMSIG(status));
        } else {
            printf("[%s] (pid %ld) termino de forma inesperada",
                   etiqueta, (long)terminado);
        }

        if (!primero_informado) {
            printf(" -- fue el primero en terminar\n");
            primero_informado = 1;
        } else {
            printf("\n");
        }

        hijos_pendientes--;
    }

    return EXIT_SUCCESS;
}
