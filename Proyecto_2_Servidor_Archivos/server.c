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
#define _GNU_SOURCE //para ppoll
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/time.h>
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
	char user[USER_MAX + 1]; /**< Usuario con el que se identifico; vacio hasta que lo haga */
}client;

/** @brief Arreglo de clientes (estatico) */
static client clients[MAXCLIENTS];

/** @brief Cantidad actual de clientes */
static int nclients = 0;

/** @brief Mutex que protege el arreglo de clientes y su contador */
static pthread_mutex_t clients_lock = PTHREAD_MUTEX_INITIALIZER;

/** @brief Carpeta donde se almacenan los archivos recibidos */
static char files_dir[PATH_MAX];

/** @brief Segundos que se espera un dato durante una subida antes de abandonar al cliente */
#define TRANSFER_TIMEOUT 30

/** @brief Segundos que se espera, al cerrar, a que terminen los hilos de los clientes */
#define SHUTDOWN_WAIT 5

/** @brief Se avisa cuando ya no queda ningun cliente */
static pthread_cond_t clients_empty = PTHREAD_COND_INITIALIZER;

/** @brief Nombres que se estan recibiendo en este momento, como USUARIO/NOMBRE (uno por cliente como maximo) */
static char busy[MAXCLIENTS][USER_MAX + NAME_MAX + 2];

/** @brief Mutex que protege la tabla de nombres en uso */
static pthread_mutex_t busy_lock = PTHREAD_MUTEX_INITIALIZER;

/** @brief Indica que el servidor debe terminar */
static volatile sig_atomic_t finished = 0;

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
 * @brief Manejador de SIGINT y SIGTERM: solo avisa que hay que terminar
 *
 * @param sig Numero de la senal
 */
void on_signal(int sig);

/**
 * @brief Reserva el nombre con el que se guardara un archivo que llega por put
 *
 * Si otro cliente esta recibiendo ahora un archivo con ese nombre, se agrega un sufijo
 * antes de la extension (client.c queda client_1.c, client_2.c...), sin pisar archivos
 * que ya existan con el sufijo.
 *
 * @param user Usuario que sube el archivo: los nombres solo chocan dentro de su carpeta
 * @param name Nombre pedido por el cliente
 * @param final Buffer que recibe el nombre reservado
 * @param size Capacidad de final
 *
 * @return 1 si se reservo un nombre, 0 si no se pudo
 */
int reserve_name(const char * user, const char * name, char * final, size_t size);

/**
 * @brief Libera un nombre reservado con reserve_name
 *
 * @param user Usuario dueño de la reserva
 * @param name Nombre reservado
 */
void release_name(const char * user, const char * name);

/**
 * @brief Pone el tiempo maximo de espera de lectura de un socket
 *
 * @param s Socket
 * @param seconds Segundos, o 0 para esperar sin limite
 */
void set_timeout(int s, int seconds);

/**
 * @brief Manda la lista de archivos de una carpeta: un file_info por archivo y uno final con size -1
 *
 * No incluye los nombres que empiezan por punto ni lo que no sea un archivo regular.
 *
 * @param s Socket del cliente
 * @param dir Carpeta del usuario
 *
 * @return 1 si se envio completa, 0 si ocurre un error
 */
int send_list(int s, const char * dir);

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
	struct sigaction sa; // Manejador de SIGINT y SIGTERM
	sigset_t block, orig; // Senales bloqueadas y mascara original
	struct pollfd pfd; // Socket de escucha para ppoll
	struct timespec deadline; // Hasta cuando se espera a los hilos al cerrar
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

	//SIGINT y SIGTERM solo activan finished. Se bloquean desde ya: los hilos que se creen
	//las heredan bloqueadas, y main las deja pasar solo mientras espera en ppoll
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = on_signal;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGTERM, &sa, NULL) == -1) {
		perror("sigaction");
		exit(EXIT_FAILURE);
	}
	sigemptyset(&block);
	sigaddset(&block, SIGINT);
	sigaddset(&block, SIGTERM);
	pthread_sigmask(SIG_BLOCK, &block, &orig);

	// Inicio de estructura repetitiva
	// para permitir la conexion con multiples clientes
	while (!finished) {
		// 4. Esperar por un cliente. ppoll desbloquea las senales solo mientras espera,
		//    asi una senal que llega justo antes no se pierde
		pfd.fd = s;
		pfd.events = POLLIN;
		pfd.revents = 0;
		if (ppoll(&pfd, 1, NULL, &orig) == -1) {
			if (errno != EINTR) {
				perror("ppoll");
				break;
			}
			continue; //llego una senal: se revisa finished
		}

		addr_len = sizeof(client_addr);
		c = accept(s, (struct sockaddr *)&client_addr, &addr_len);
		if (c == -1) {
			if (errno != EINTR && errno != ECONNABORTED && errno != EAGAIN) {
				perror("accept");
				usleep(100000); //sin descriptores u otro error que no pasa solo: no girar en vacio
			}
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

	// 7. Cerrar el socket obtenido en el paso 1 y despedir a los clientes
	printf("Cerrando servidor...\n");
	close(s);

	//shutdown despierta a los hilos que esperan en un read; ellos mismos limpian y salen
	pthread_mutex_lock(&clients_lock);
	for (i = 0; i < MAXCLIENTS; i++) {
		if (clients[i].socket != -1) {
			shutdown(clients[i].socket, SHUT_RDWR);
		}
	}
	clock_gettime(CLOCK_REALTIME, &deadline);
	deadline.tv_sec += SHUTDOWN_WAIT;
	while (nclients > 0) {
		if (pthread_cond_timedwait(&clients_empty, &clients_lock, &deadline) == ETIMEDOUT) {
			break;
		}
	}
	rc = nclients;
	pthread_mutex_unlock(&clients_lock);

	if (rc > 0) {
		printf("Quedaron %d clientes sin terminar\n", rc);
	}

	exit(EXIT_SUCCESS);
}

void on_signal(int sig) {
	(void)sig;
	finished = 1;
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
			cl->user[0] = '\0'; //la posicion puede venir de otro cliente: sin usuario hasta que se identifique
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
	if (nclients == 0) {
		pthread_cond_broadcast(&clients_empty); //por si main espera para cerrar
	}
	pthread_mutex_unlock(&clients_lock);

	printf("Cliente %s:%d desconectado (posicion %d, %d en total)\n", ip, port, index, total);
}

void * client_thread(void * arg) {
	client * c = (client *)arg;
	request req; // Solicitud del cliente
	char msg[256]; // Mensaje de error para el cliente
	char udir[PATH_MAX]; // Carpeta del usuario: files/USUARIO
	char path[PATH_MAX]; // Ruta del archivo pedido con get
	struct stat st; // Datos del archivo pedido
	char stored[NAME_MAX]; // Nombre con el que se guarda un put
	int ok; // Resultado de receive_file

	//Leer solicitudes de tamaño fijo hasta que el cliente cierre o envie exit
	while (read_all(c->socket, &req, sizeof(req)) == (ssize_t)sizeof(req)) {
		//Garantizar que las cadenas recibidas terminen en nulo
		req.operation[sizeof(req.operation) - 1] = '\0';
		req.filename[sizeof(req.filename) - 1] = '\0';
		lcase(req.operation);

		if (equals(req.operation, "exit")) {
			break;
		}

		//lo primero que debe hacer el cliente es decir quien es: cada usuario tiene su carpeta
		if (equals(req.operation, "user")) {
			if (c->user[0] != '\0') {
				ok = reply(c->socket, TRANSFER_DENIED, "ya esta identificado");
			} else if (!valid_user(req.filename)) {
				ok = reply(c->socket, TRANSFER_DENIED, "usuario no valido");
			} else if (snprintf(udir, sizeof(udir), "%s/%s", files_dir, req.filename) >= (int)sizeof(udir)
				|| !ensure_dir(udir)) {
				ok = reply(c->socket, TRANSFER_DENIED, "no se pudo crear la carpeta del usuario");
			} else {
				snprintf(c->user, sizeof(c->user), "%.*s", USER_MAX, req.filename);
				printf("Cliente %s:%d: usuario %s\n", c->ip, ntohs(c->address.sin_port), c->user);
				ok = reply(c->socket, TRANSFER_AUTHORIZED, "identificado");
			}
			if (!ok) {
				break;
			}
			continue;
		}

		if (c->user[0] == '\0') {
			if (!reply(c->socket, TRANSFER_DENIED, "primero hay que identificarse con un usuario")) {
				break;
			}
			continue;
		}
		//a partir de aqui todo ocurre dentro de la carpeta del usuario, no se ve la de los demas
		if (snprintf(udir, sizeof(udir), "%s/%s", files_dir, c->user) >= (int)sizeof(udir)) {
			break; //la ruta no cabe: no deberia pasar, ya se comprobo al identificarse
		}

		if (equals(req.operation, "list")) {
			if (!send_list(c->socket, udir)) {
				break;
			}
			continue;
		}

		if (equals(req.operation, "put")) {
			//el nombre se revisa antes de aceptar el archivo
			if (!valid_filename(req.filename)) {
				if (!reply(c->socket, TRANSFER_DENIED, "nombre de archivo no valido")) {
					break;
				}
				continue;
			}
			//si ese mismo usuario esta subiendo ahora el mismo nombre, este se guarda con sufijo
			if (!reserve_name(c->user, req.filename, stored, sizeof(stored))) {
				if (!reply(c->socket, TRANSFER_DENIED, "no se pudo reservar el nombre")) {
					break;
				}
				continue;
			}
			if (equals(stored, req.filename)) {
				snprintf(msg, sizeof(msg), "listo para recibir");
			} else {
				snprintf(msg, sizeof(msg), "listo para recibir, se guardara como %.200s", stored);
			}
			if (!reply(c->socket, TRANSFER_AUTHORIZED, msg)) {
				release_name(c->user, stored);
				break;
			}

			//ahora el cliente manda la informacion del archivo y su contenido; si deja de
			//mandar datos mas de TRANSFER_TIMEOUT segundos se le corta la conexion
			set_timeout(c->socket, TRANSFER_TIMEOUT);
			ok = receive_file(c->socket, udir, stored);
			set_timeout(c->socket, 0);
			release_name(c->user, stored);

			if (ok != 1) {
				//no se sabe en que punto quedo el flujo: se avisa y se cierra
				reply(c->socket, TRANSFER_DENIED, "no se pudo recibir el archivo");
				break;
			}
			printf("Cliente %s:%d (%s): recibido %s\n", c->ip, ntohs(c->address.sin_port), c->user, stored);
			if (!reply(c->socket, TRANSFER_AUTHORIZED, "archivo recibido")) {
				break;
			}
			continue;
		}

		if (equals(req.operation, "get")) {
			//si el nombre no sirve o el archivo no existe (o es un enlace), se avisa con size -1
			if (!valid_filename(req.filename) ||
				snprintf(path, sizeof(path), "%s/%s", udir, req.filename) >= (int)sizeof(path) ||
				lstat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
				if (!send_not_found(c->socket, req.filename)) {
					break;
				}
				continue;
			}
			if (!send_file(c->socket, path)) {
				break; //el cliente ya espera el contenido: no hay forma de seguir
			}
			printf("Cliente %s:%d (%s): enviado %s\n", c->ip, ntohs(c->address.sin_port), c->user, req.filename);
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

int send_list(int s, const char * dir) {
	struct dirent ** names;
	struct stat st;
	file_info info;
	char path[PATH_MAX];
	int n, i, sent;

	//se manda un file_info por archivo regular y, al final, uno con size -1
	n = scandir(dir, &names, NULL, alphasort);
	if (n < 0) {
		return send_not_found(s, ""); //carpeta ilegible: lista vacia
	}

	sent = 1;
	for (i = 0; i < n; i++) {
		if (sent && names[i]->d_name[0] != '.' &&
			snprintf(path, sizeof(path), "%s/%s", dir, names[i]->d_name) < (int)sizeof(path) &&
			lstat(path, &st) == 0 && S_ISREG(st.st_mode)) {
			memset(&info, 0, sizeof(info));
			info.size = st.st_size;
			info.mode = st.st_mode;
			strncpy(info.filename, names[i]->d_name, sizeof(info.filename) - 1);
			sent = (write_all(s, &info, sizeof(info)) == (ssize_t)sizeof(info));
		}
		free(names[i]);
	}
	free(names);

	return sent && send_not_found(s, ""); //el ultimo mensaje, con size -1, cierra la lista
}

int reserve_name(const char * user, const char * name, char * final, size_t size) {
	char candidate[NAME_MAX];
	char key[USER_MAX + NAME_MAX + 2];
	char path[PATH_MAX];
	const char * dot;
	int n, i, free_slot, taken;

	//si hay punto, el sufijo va antes de la extension (client.c -> client_1.c)
	dot = strrchr(name, '.');
	if (dot == NULL) {
		dot = name + strlen(name);
	}

	pthread_mutex_lock(&busy_lock);
	for (n = 0; n < 1000; n++) {
		if (n == 0) {
			snprintf(candidate, sizeof(candidate), "%s", name);
		} else if (snprintf(candidate, sizeof(candidate), "%.*s_%d%s",
				(int)(dot - name), name, n, dot) >= (int)sizeof(candidate)) {
			break; //con el sufijo el nombre no cabe
		}

		//la tabla guarda USUARIO/NOMBRE: dos usuarios distintos nunca chocan
		snprintf(key, sizeof(key), "%s/%s", user, candidate);
		taken = 0;
		for (i = 0; i < MAXCLIENTS; i++) {
			if (equals(busy[i], key)) {
				taken = 1;
			}
		}
		//el nombre original puede sobrescribir uno que ya existe; los que llevan sufijo no
		if (!taken && n > 0) {
			taken = (snprintf(path, sizeof(path), "%s/%s/%s", files_dir, user, candidate) >= (int)sizeof(path)
				|| access(path, F_OK) == 0);
		}
		if (taken) {
			continue;
		}

		for (free_slot = 0; free_slot < MAXCLIENTS && busy[free_slot][0] != '\0'; free_slot++);
		if (free_slot == MAXCLIENTS || strlen(candidate) >= size) {
			break;
		}
		strcpy(busy[free_slot], key);
		strcpy(final, candidate);
		pthread_mutex_unlock(&busy_lock);
		return 1;
	}
	pthread_mutex_unlock(&busy_lock);

	return 0;
}

void release_name(const char * user, const char * name) {
	char key[USER_MAX + NAME_MAX + 2];
	int i;

	snprintf(key, sizeof(key), "%s/%s", user, name);
	pthread_mutex_lock(&busy_lock);
	for (i = 0; i < MAXCLIENTS; i++) {
		if (equals(busy[i], key)) {
			busy[i][0] = '\0';
			break;
		}
	}
	pthread_mutex_unlock(&busy_lock);
}

void set_timeout(int s, int seconds) {
	struct timeval tv;

	tv.tv_sec = seconds;
	tv.tv_usec = 0;
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}
