/**
 * @file
 * @brief Servidor TCP concurrente con un recurso compartido entre lectores y escritores.
 *
 * El servidor mantiene un tablero de texto en memoria. Atiende cada conexion en un hilo
 * independiente, de modo que varios clientes operan sobre el tablero al mismo tiempo. La
 * politica de sincronizacion se elige al arrancar:
 *
 *   sin     - ninguna proteccion sobre el tablero
 *   mutex   - exclusion mutua total: un solo cliente a la vez, lea o escriba
 *   rwlock  - lectores y escritores: varias lecturas simultaneas, escritura exclusiva
 *
 * Protocolo de texto, una orden por linea:
 *
 *   LEER              -> OK \<version\> \<contenido del tablero\>
 *   ESCRIBIR \<car\>    -> OK \<version\>
 *   SALIR             -> cierra la conexion
 *
 * Manejo de senales (taller de senales):
 *
 *   SIGINT  - termina el servidor de forma ordenada: se cierra el socket de escucha y se
 *             presentan las lecturas y escrituras atendidas.
 *   SIGALRM - cada INTERVALO_INFORME segundos el servidor presenta la version y el contenido
 *             del tablero. La alarma se vuelve a programar despues de cada informe.
 *
 * Los manejadores solo registran el hecho en una bandera; el trabajo lo hace el ciclo
 * principal cuando observa la bandera.
 *
 * @author Erwin Meza Vega <emezav@unicauca.edu.co>
 * @author Edward Esteban Davila Salazar
 * @author Miguel Angel Perez Mera
 * @copyright MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <stdatomic.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

/** @brief Puerto en el que el servidor recibe conexiones. */
#define PUERTO 5500

/** @brief Numero de casillas del tablero compartido. */
#define TAM_TABLERO 64

/** @brief Tamano del buffer de ordenes y respuestas. */
#define TAM_BUFFER 256

/** @brief Microsegundos de espera entre casilla y casilla al escribir. */
#define RETARDO_ESCRITURA 2000

/** @brief Microsegundos de espera entre casilla y casilla al leer. */
#define RETARDO_LECTURA 200

/** @brief Segundos entre dos informes periodicos del estado del tablero. */
#define INTERVALO_INFORME 5

/** @brief Politicas de sincronizacion disponibles. */
typedef enum {
    SIN_PROTECCION, /**< El tablero queda expuesto a accesos simultaneos. */
    CON_MUTEX,      /**< Un solo cliente accede al tablero, lea o escriba. */
    CON_RWLOCK      /**< Varios lectores a la vez, un escritor en exclusiva. */
} politica;

/** @brief Recurso compartido: todas las casillas del tablero contienen el mismo caracter. */
static char tablero[TAM_TABLERO];

/** @brief Numero de la ultima escritura completada. */
static int version_tablero;

/** @brief Candado de la politica mutex. */
static pthread_mutex_t candado = PTHREAD_MUTEX_INITIALIZER;

/** @brief Candado de la politica rwlock. */
static pthread_rwlock_t candado_rw = PTHREAD_RWLOCK_INITIALIZER;

/** @brief Politica en uso durante esta ejecucion. */
static politica politica_actual = SIN_PROTECCION;

/** @brief Bandera que el manejador de SIGINT activa para pedir la terminacion ordenada. */
static volatile sig_atomic_t terminar = 0;

/** @brief Bandera que el manejador de SIGALRM activa para pedir un informe del tablero. */
static volatile sig_atomic_t informar = 0;

/**
 * @brief Lecturas (orden LEER) atendidas por todos los hilos.
 *
 * Es memoria compartida entre los hilos de los clientes. Cada incremento es una operacion
 * atomica: ver la bitacora para el criterio con que se eligio este mecanismo.
 */
static atomic_long lecturas_atendidas;

/** @brief Escrituras (orden ESCRIBIR) atendidas por todos los hilos. */
static atomic_long escrituras_atendidas;

/**
 * @brief Manejador de SIGINT: solo registra que se pidio terminar.
 * @param senal Numero de la senal recibida, que no se usa.
 */
static void al_interrumpir(int senal);

/**
 * @brief Manejador de SIGALRM: solo registra que se debe presentar el estado del tablero.
 * @param senal Numero de la senal recibida, que no se usa.
 */
static void al_sonar_alarma(int senal);

/**
 * @brief Instala un manejador de senal con sigaction y sin SA_RESTART.
 *
 * Sin SA_RESTART, una llamada bloqueante interrumpida por la senal (como accept) retorna -1
 * con errno igual a EINTR, y el ciclo principal recupera el control.
 *
 * @param senal Numero de la senal que se captura.
 * @param manejador Funcion que se ejecuta al recibirla.
 * @return 1 si el manejador quedo instalado, 0 si sigaction fallo (ya informado con perror).
 */
int instalar_manejador(int senal, void (*manejador)(int));

/**
 * @brief Presenta la version del tablero y su contenido.
 *
 * La lectura aplica la politica vigente en modo de lectura, igual que la orden LEER.
 */
void presentar_estado(void);

/**
 * @brief Presenta cuantas lecturas y cuantas escrituras atendio el servidor.
 */
void presentar_contadores(void);

/**
 * @brief Crea el hilo que atiende a un cliente, sin que este reciba SIGINT ni SIGALRM.
 *
 * Una senal dirigida al proceso puede entregarse a cualquier hilo que no la bloquee. Los
 * hilos de los clientes la bloquean, de modo que solo el hilo principal (el que esta en
 * accept) la recibe. El hilo hereda la mascara que tiene su creador al nacer, por eso las
 * senales se bloquean antes de pthread_create y se restauran justo despues.
 *
 * @param descriptor Apuntador al descriptor del cliente; el hilo lo libera al arrancar.
 * @return 1 si el hilo quedo creado, 0 en caso contrario (ya informado).
 */
int crear_hilo_cliente(int *descriptor);

/**
 * @brief Informa el fallo de una funcion de pthread, que devuelve el codigo de error.
 *
 * Las funciones de pthread no usan errno, de modo que perror no sirve con ellas.
 *
 * @param llamada Nombre de la funcion que fallo.
 * @param codigo Codigo de error que devolvio.
 */
void informar_error_pthread(const char *llamada, int codigo);

/**
 * @brief Traduce el nombre de la politica recibido por linea de comandos.
 * @param nombre Cadena con el nombre de la politica.
 * @param destino Apuntador a la variable que recibe la politica reconocida.
 * @return 1 si el nombre corresponde a una politica conocida, 0 en caso contrario.
 */
int reconocer_politica(const char *nombre, politica *destino);

/**
 * @brief Crea el socket de escucha del servidor.
 * @param puerto Puerto en el que se reciben las conexiones.
 * @return Descriptor del socket de escucha, o -1 si ocurre un error.
 */
int crear_socket_escucha(int puerto);

/**
 * @brief Atiende las ordenes de un cliente hasta que cierra la conexion.
 * @param arg Apuntador al descriptor del socket del cliente, reservado por el llamador.
 * @return NULL siempre.
 */
void *atender_cliente(void *arg);

/**
 * @brief Lee una linea terminada en salto de linea desde un socket.
 * @param socket_cliente Descriptor del socket.
 * @param destino Buffer que recibe la linea, sin el salto de linea final.
 * @param tam Tamano del buffer de destino.
 * @return Numero de caracteres leidos, o -1 si el cliente cerro la conexion o hubo error.
 */
int leer_linea(int socket_cliente, char *destino, int tam);

/**
 * @brief Copia el contenido del tablero aplicando la politica de sincronizacion vigente.
 * @param destino Buffer que recibe la copia, con al menos TAM_TABLERO + 1 caracteres.
 * @return Version del tablero observada durante la lectura.
 */
int leer_tablero(char *destino);

/**
 * @brief Llena el tablero con un caracter aplicando la politica de sincronizacion vigente.
 * @param caracter Caracter que se escribe en todas las casillas.
 * @return Version del tablero despues de la escritura.
 */
int escribir_tablero(char caracter);

/**
 * @brief Punto de entrada: instala las senales, atiende conexiones y termina de forma ordenada.
 * @param argc Numero de argumentos de la linea de comandos.
 * @param argv Argumentos: la politica de sincronizacion (sin, mutex o rwlock).
 * @return EXIT_SUCCESS si el servidor termino de forma ordenada, EXIT_FAILURE si ocurrio un error.
 */
int main(int argc, char *argv[]) {
    int socket_escucha;
    int socket_cliente;
    int *descriptor;
    int estado = EXIT_SUCCESS;

    if (argc != 2 || !reconocer_politica(argv[1], &politica_actual)) {
        fprintf(stderr, "Uso: %s sin|mutex|rwlock\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Un cliente que cierra de golpe no debe terminar el servidor */
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("signal");
        return EXIT_FAILURE;
    }

    /* Los manejadores quedan instalados antes de aceptar la primera conexion */
    if (!instalar_manejador(SIGINT, al_interrumpir) ||
        !instalar_manejador(SIGALRM, al_sonar_alarma)) {
        return EXIT_FAILURE;
    }

    memset(tablero, '.', TAM_TABLERO);
    version_tablero = 0;

    socket_escucha = crear_socket_escucha(PUERTO);
    if (socket_escucha == -1) {
        return EXIT_FAILURE;
    }

    printf("Tablero de %d casillas, politica %s, escuchando en el puerto %d\n",
           TAM_TABLERO, argv[1], PUERTO);
    fflush(stdout);

    alarm(INTERVALO_INFORME);

    while (!terminar) {
        /* El informe se presenta aqui y no en el manejador: leer el tablero toma el
           candado y usa printf, que no estan permitidos dentro de un manejador */
        if (informar) {
            informar = 0;
            presentar_estado();
            alarm(INTERVALO_INFORME);
        }

        socket_cliente = accept(socket_escucha, NULL, NULL);
        if (socket_cliente == -1) {
            /* Una senal interrumpio accept: no es un error, se vuelven a mirar las banderas */
            if (errno == EINTR) {
                continue;
            }
            perror("accept");
            continue;
        }

        /* El hilo recibe su propia copia del descriptor y la libera al terminar */
        descriptor = malloc(sizeof(int));
        if (descriptor == NULL) {
            perror("malloc");
            close(socket_cliente);
            continue;
        }
        *descriptor = socket_cliente;

        if (!crear_hilo_cliente(descriptor)) {
            free(descriptor);
            close(socket_cliente);
            continue;
        }
    }

    printf("\nSenal de terminacion recibida: se cierra el servidor\n");
    if (close(socket_escucha) == -1) {
        perror("close");
        estado = EXIT_FAILURE;
    }
    presentar_contadores();
    fflush(stdout);

    return estado;
}

static void al_interrumpir(int senal) {
    (void) senal;
    terminar = 1;
}

static void al_sonar_alarma(int senal) {
    (void) senal;
    informar = 1;
}

int instalar_manejador(int senal, void (*manejador)(int)) {
    struct sigaction accion;

    memset(&accion, 0, sizeof(accion));
    accion.sa_handler = manejador;
    sigemptyset(&accion.sa_mask);
    accion.sa_flags = 0; /* sin SA_RESTART: accept debe retornar con EINTR */

    if (sigaction(senal, &accion, NULL) == -1) {
        perror("sigaction");
        return 0;
    }
    return 1;
}

void presentar_estado(void) {
    char copia[TAM_TABLERO + 1];
    int version;

    version = leer_tablero(copia);
    printf("[informe] version %d, tablero: %s\n", version, copia);
    fflush(stdout);
}

void presentar_contadores(void) {
    printf("Lecturas atendidas: %ld\n", atomic_load(&lecturas_atendidas));
    printf("Escrituras atendidas: %ld\n", atomic_load(&escrituras_atendidas));
}

void informar_error_pthread(const char *llamada, int codigo) {
    fprintf(stderr, "%s: %s\n", llamada, strerror(codigo));
}

int crear_hilo_cliente(int *descriptor) {
    sigset_t bloqueadas;
    sigset_t anteriores;
    pthread_t hilo;
    int codigo;
    int creado = 0;

    sigemptyset(&bloqueadas);
    sigaddset(&bloqueadas, SIGINT);
    sigaddset(&bloqueadas, SIGALRM);

    codigo = pthread_sigmask(SIG_BLOCK, &bloqueadas, &anteriores);
    if (codigo != 0) {
        informar_error_pthread("pthread_sigmask", codigo);
        return 0;
    }

    codigo = pthread_create(&hilo, NULL, atender_cliente, descriptor);
    if (codigo != 0) {
        informar_error_pthread("pthread_create", codigo);
    } else {
        creado = 1;

        /* Nadie espera el resultado del hilo: sus recursos se liberan al terminar */
        codigo = pthread_detach(hilo);
        if (codigo != 0) {
            informar_error_pthread("pthread_detach", codigo);
        }
    }

    codigo = pthread_sigmask(SIG_SETMASK, &anteriores, NULL);
    if (codigo != 0) {
        informar_error_pthread("pthread_sigmask", codigo);
    }

    return creado;
}

int reconocer_politica(const char *nombre, politica *destino) {
    if (strcmp(nombre, "sin") == 0) {
        *destino = SIN_PROTECCION;
        return 1;
    }
    if (strcmp(nombre, "mutex") == 0) {
        *destino = CON_MUTEX;
        return 1;
    }
    if (strcmp(nombre, "rwlock") == 0) {
        *destino = CON_RWLOCK;
        return 1;
    }
    return 0;
}

int crear_socket_escucha(int puerto) {
    int socket_escucha;
    int reutilizar = 1;
    struct sockaddr_in direccion;

    socket_escucha = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_escucha == -1) {
        perror("socket");
        return -1;
    }

    /* El puerto queda disponible de inmediato al terminar el servidor */
    if (setsockopt(socket_escucha, SOL_SOCKET, SO_REUSEADDR, &reutilizar,
                   sizeof(reutilizar)) == -1) {
        perror("setsockopt");
        close(socket_escucha);
        return -1;
    }

    memset(&direccion, 0, sizeof(direccion));
    direccion.sin_family = AF_INET;
    direccion.sin_addr.s_addr = htonl(INADDR_ANY);
    direccion.sin_port = htons(puerto);

    if (bind(socket_escucha, (struct sockaddr *) &direccion, sizeof(direccion)) == -1) {
        perror("bind");
        close(socket_escucha);
        return -1;
    }

    if (listen(socket_escucha, 16) == -1) {
        perror("listen");
        close(socket_escucha);
        return -1;
    }

    return socket_escucha;
}

void *atender_cliente(void *arg) {
    int socket_cliente = *((int *) arg);
    char orden[TAM_BUFFER];
    char copia[TAM_TABLERO + 1];
    char respuesta[TAM_BUFFER];
    int version;
    int longitud;

    free(arg);

    while (1) {
        longitud = leer_linea(socket_cliente, orden, TAM_BUFFER);
        if (longitud == -1) {
            break;
        }

        if (strcmp(orden, "LEER") == 0) {
            version = leer_tablero(copia);
            atomic_fetch_add(&lecturas_atendidas, 1);
            snprintf(respuesta, TAM_BUFFER, "OK %d %s\n", version, copia);
        } else if (strncmp(orden, "ESCRIBIR ", 9) == 0 && orden[9] != '\0') {
            version = escribir_tablero(orden[9]);
            atomic_fetch_add(&escrituras_atendidas, 1);
            snprintf(respuesta, TAM_BUFFER, "OK %d\n", version);
        } else if (strcmp(orden, "SALIR") == 0) {
            break;
        } else {
            snprintf(respuesta, TAM_BUFFER, "ERROR orden no reconocida\n");
        }

        if (write(socket_cliente, respuesta, strlen(respuesta)) == -1) {
            break;
        }
    }

    close(socket_cliente);
    return NULL;
}

int leer_linea(int socket_cliente, char *destino, int tam) {
    int total = 0;
    char caracter;
    ssize_t leidos;

    while (total < tam - 1) {
        leidos = read(socket_cliente, &caracter, 1);

        /* El cliente cerro la conexion: una linea a medias no se procesa */
        if (leidos <= 0) {
            return -1;
        }
        if (caracter == '\n') {
            break;
        }
        if (caracter != '\r') {
            destino[total] = caracter;
            total++;
        }
    }

    destino[total] = '\0';
    return total;
}

int leer_tablero(char *destino) {
    int i;
    int version;

    if (politica_actual == CON_MUTEX) {
        pthread_mutex_lock(&candado);
    } else if (politica_actual == CON_RWLOCK) {
        pthread_rwlock_rdlock(&candado_rw);
    }

    /* La copia casilla por casilla amplia la ventana en la que otro hilo puede intervenir */
    for (i = 0; i < TAM_TABLERO; i++) {
        destino[i] = tablero[i];
        usleep(RETARDO_LECTURA);
    }
    destino[TAM_TABLERO] = '\0';
    version = version_tablero;

    if (politica_actual == CON_MUTEX) {
        pthread_mutex_unlock(&candado);
    } else if (politica_actual == CON_RWLOCK) {
        pthread_rwlock_unlock(&candado_rw);
    }

    return version;
}

int escribir_tablero(char caracter) {
    int i;
    int version;

    if (politica_actual == CON_MUTEX) {
        pthread_mutex_lock(&candado);
    } else if (politica_actual == CON_RWLOCK) {
        pthread_rwlock_wrlock(&candado_rw);
    }

    for (i = 0; i < TAM_TABLERO; i++) {
        tablero[i] = caracter;
        usleep(RETARDO_ESCRITURA);
    }
    version_tablero++;
    version = version_tablero;

    if (politica_actual == CON_MUTEX) {
        pthread_mutex_unlock(&candado);
    } else if (politica_actual == CON_RWLOCK) {
        pthread_rwlock_unlock(&candado_rw);
    }

    return version;
}
