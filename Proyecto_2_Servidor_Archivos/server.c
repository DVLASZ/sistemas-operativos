/**
* @file
* @brief Servidor de archivos.
*
* Atiende varios clientes a la vez, un hilo por conexion. Los clientes se registran en un
* arreglo protegido con un mutex y se quitan de el cuando terminan.
*
* @author Erwin Meza Vega <emezav@unicauca.edu.co>
* @author Edward Esteban Dávila Salazar
* @author Miguel Angel Perez Mera
* @copyright MIT License
*/
#include <arpa/inet.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/ip.h>
#include <unistd.h>

#include "protocol.h"
#include "fsutil.h"
#include "util.h"
#include "netutil.h"

/** @brief Capacidad del arreglo de clientes */
#define MAXCLIENTS 256

/**
 * @brief Cliente
 */
typedef struct {
	int socket; /**< Descriptor del socket; -1 si la posicion esta libre */
	int index; /**< Indice dentro del arreglo de clientes*/
	struct sockaddr_in address; /**< Direccion del cliente */
	char ip[INET_ADDRSTRLEN]; /**< Direccion IP en formato aaa.bbb.ccc.ddd */
}client;

/** @brief Arreglo de clientes (estatico) */
static client clients[MAXCLIENTS];

/** @brief Cantidad actual de clientes */
static int nclients = 0;

/** @brief Mutex que protege el arreglo de clientes y su contador */
static pthread_mutex_t clients_lock = PTHREAD_MUTEX_INITIALIZER;

/** @brief Carpeta donde se almacenan los archivos recibidos */
static char files_dir[PATH_MAX];

/** @brief Indica que el servidor debe terminar */
static volatile int finished = 0;

/**
 * @brief Hilo de cliente: atiende los comandos de un cliente hasta que se desconecta.
 *
 * @param arg Apuntador al cliente (posicion en el arreglo de clientes)
 *
 * @return NULL
 */
void * client_thread(void * arg);

/**
 * @brief Adiciona un socket de cliente al arreglo de clientes
 *
 * @param c Socket de conexion del cliente
 * @param addr Referencia a la direccion del cliente
 *
 * @return Apuntador a la posicion del cliente, o NULL si el arreglo esta lleno
 */
client * add_client(int c, struct sockaddr_in * addr);

/**
 * @brief Elimina un cliente: cierra su socket y libera su posicion en el arreglo
 *
 * @param c Apuntador al cliente
 */
void remove_client(client * c);

/**
 * @brief Interpreta un puerto recibido por linea de comandos
 *
 * @param text Texto con el numero de puerto
 *
 * @return El puerto (1 a 65535), o 0 si el texto no es un puerto valido
 */
unsigned short parse_port(const char * text);

/**
 * @brief Envia una respuesta al cliente
 *
 * @param s Socket del cliente
 * @param code Codigo de respuesta
 * @param message Mensaje para el cliente
 *
 * @return 1 si se envio completa, 0 si ocurre un error
 */
int reply(int s, response_code code, const char * message);

int main(int argc, char * argv[]) {
	int s, c; // Conectores
	int opt = 1; // Valor para SO_REUSEADDR
	unsigned short port = DEFAULT_PORT; // Puerto del servidor
	char base[PATH_MAX]; // Carpeta donde esta el programa
	struct sockaddr_in * addr; //Direccion IPv4 del socket del servidor
	struct sockaddr_in client_addr; //Direccion IPv4 del cliente
	socklen_t addr_len; // Tamaño de la direccion
	client * cl; // Cliente registrado
	pthread_t tid; // Hilo del cliente
	int rc; // Resultado de pthread_create
	int i;

	if (argc > 2) {
		fprintf(stderr, "Uso: %s [puerto]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	if (argc == 2) {
		port = parse_port(argv[1]);
		if (port == 0) {
			fprintf(stderr, "Puerto invalido: %s\n", argv[1]);
			exit(EXIT_FAILURE);
		}
	}

	//Mensajes linea por linea, tambien cuando la salida se redirige a un archivo
	setvbuf(stdout, NULL, _IOLBF, 0);

	//Un cliente que cierra de golpe no debe terminar el servidor (SIGPIPE)
	signal(SIGPIPE, SIG_IGN);

	//Todas las posiciones del arreglo de clientes comienzan libres
	for (i = 0; i < MAXCLIENTS; i++) {
		clients[i].socket = -1;
	}

	//La carpeta files se crea junto al programa, si no existe
	if (!program_dir(base, sizeof(base))) {
		fprintf(stderr, "No se pudo determinar la carpeta del programa\n");
		exit(EXIT_FAILURE);
	}
	if (snprintf(files_dir, sizeof(files_dir), "%s/files", base) >= (int)sizeof(files_dir)) {
		fprintf(stderr, "La ruta del programa es demasiado larga\n");
		exit(EXIT_FAILURE);
	}
	if (!ensure_dir(files_dir)) {
		perror("No se pudo crear la carpeta de archivos");
		exit(EXIT_FAILURE);
	}

	// 1. Obtener un conector IPv4
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}
	//Permite reiniciar el servidor sin esperar a que se libere el puerto
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	// 2. Asociar el socket a una direccion IPv4 (cualquier interfaz)
	addr = any_address(port);
	if (addr == NULL) {
		perror("any_address");
		exit(EXIT_FAILURE);
	}
	if (bind(s, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) == -1) {
		perror("bind");
		exit(EXIT_FAILURE);
	}
	free(addr);

	// 3. Poner el socket disponible
	if (listen(s, 10) == -1) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	printf("Servidor de archivos en el puerto %d, carpeta %s\n", port, files_dir);

	// Inicio de estructura repetitiva
	// para permitir la conexion con multiples clientes
	while (!finished) {
		// 4. Esperar por un cliente
		addr_len = sizeof(client_addr);
		c = accept(s, (struct sockaddr *)&client_addr, &addr_len);
		if (c == -1) {
			perror("accept");
			continue;
		}

		//Registrar al cliente en el arreglo (bajo el mutex)
		cl = add_client(c, &client_addr);
		if (cl == NULL) {
			fprintf(stderr, "Servidor lleno: se rechaza la conexion\n");
			close(c);
			continue;
		}

		//Un hilo por cada conexion. Recibe la posicion del cliente en el arreglo; es seguro porque
		//solo ese hilo la libera (remove_client) y main no la reutiliza hasta entonces.
		rc = pthread_create(&tid, NULL, client_thread, cl);
		if (rc != 0) {
			fprintf(stderr, "pthread_create: %s\n", strerror(rc));
			remove_client(cl);
			continue;
		}
		//Nadie espera al hilo: sus recursos se liberan solos al terminar
		pthread_detach(tid);
	}

	// 7. Cerrar el socket obtenido en el paso 1.
	printf("Cerrando socket...\n");
	close(s);

	exit(EXIT_SUCCESS);
}

unsigned short parse_port(const char * text) {
	char * end;
	long value;

	value = strtol(text, &end, 10);
	if (*text == '\0' || *end != '\0' || value < 1 || value > 65535) {
		return 0;
	}
	return (unsigned short)value;
}

client * add_client(int c, struct sockaddr_in * addr) {
	int i;
	int total = 0; // Cantidad de clientes despues de registrar
	client * cl = NULL;

	//Seccion critica: un solo hilo a la vez modifica el arreglo de clientes
	pthread_mutex_lock(&clients_lock);
	for (i = 0; i < MAXCLIENTS; i++) {
		if (clients[i].socket == -1) {
			cl = &clients[i];
			cl->socket = c;
			cl->index = i;
			cl->address = *addr;
			inet_ntop(AF_INET, &addr->sin_addr, cl->ip, INET_ADDRSTRLEN);
			nclients++;
			total = nclients;
			break;
		}
	}
	pthread_mutex_unlock(&clients_lock);

	//Se escribe fuera de la seccion critica: si la salida se bloquea no frena a los demas hilos
	if (cl != NULL) {
		printf("Cliente %s:%d conectado (posicion %d, %d en total)\n",
			cl->ip, ntohs(addr->sin_port), cl->index, total);
	}

	return cl;
}

void remove_client(client * c) {
	char ip[INET_ADDRSTRLEN]; // Copias para informar despues de liberar la posicion
	int port, index, total;

	strcpy(ip, c->ip);
	port = ntohs(c->address.sin_port);
	index = c->index;

	pthread_mutex_lock(&clients_lock);
	close(c->socket);
	c->socket = -1; //La posicion queda libre
	nclients--;
	total = nclients;
	pthread_mutex_unlock(&clients_lock);

	printf("Cliente %s:%d desconectado (posicion %d, %d en total)\n", ip, port, index, total);
}

void * client_thread(void * arg) {
	client * c = (client *)arg;
	request req; // Solicitud del cliente
	char msg[256]; // Mensaje de error para el cliente
	char path[PATH_MAX]; // Ruta del archivo pedido con get
	struct stat st; // Datos del archivo pedido

	//Leer solicitudes de tamaño fijo hasta que el cliente cierre o envie exit
	while (read_all(c->socket, &req, sizeof(req)) == (ssize_t)sizeof(req)) {
		//Garantizar que las cadenas recibidas terminen en nulo
		req.operation[sizeof(req.operation) - 1] = '\0';
		req.filename[sizeof(req.filename) - 1] = '\0';
		lcase(req.operation);

		if (equals(req.operation, "exit")) {
			break;
		}

		if (equals(req.operation, "put")) {
			//el nombre se revisa antes de aceptar el archivo
			if (!valid_filename(req.filename)) {
				if (!reply(c->socket, TRANSFER_DENIED, "nombre de archivo no valido")) {
					break;
				}
				continue;
			}
			if (!reply(c->socket, TRANSFER_AUTHORIZED, "listo para recibir")) {
				break;
			}
			//ahora el cliente manda la informacion del archivo y su contenido
			if (receive_file(c->socket, files_dir) != 1) {
				//no se sabe en que punto quedo el flujo: se avisa y se cierra
				reply(c->socket, TRANSFER_DENIED, "no se pudo recibir el archivo");
				break;
			}
			printf("Cliente %s:%d: recibido %s\n", c->ip, ntohs(c->address.sin_port), req.filename);
			if (!reply(c->socket, TRANSFER_AUTHORIZED, "archivo recibido")) {
				break;
			}
			continue;
		}

		if (equals(req.operation, "get")) {
			//si el nombre no sirve o el archivo no existe (o es un enlace), se avisa con size -1
			if (!valid_filename(req.filename) ||
				snprintf(path, sizeof(path), "%s/%s", files_dir, req.filename) >= (int)sizeof(path) ||
				lstat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
				if (!send_not_found(c->socket, req.filename)) {
					break;
				}
				continue;
			}
			if (!send_file(c->socket, path)) {
				break; //el cliente ya espera el contenido: no hay forma de seguir
			}
			printf("Cliente %s:%d: enviado %s\n", c->ip, ntohs(c->address.sin_port), req.filename);
			continue;
		}

		snprintf(msg, sizeof(msg), "comando desconocido: %.100s", req.operation);
		if (!reply(c->socket, TRANSFER_DENIED, msg)) {
			break;
		}
	}

	remove_client(c);
	return NULL;
}

int reply(int s, response_code code, const char * message) {
	response res;

	memset(&res, 0, sizeof(res));
	res.code = code;
	snprintf(res.message, sizeof(res.message), "%s", message);

	return write_all(s, &res, sizeof(res)) == (ssize_t)sizeof(res);
}
