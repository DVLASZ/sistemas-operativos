/**
* @file
* @brief Cliente de archivos.
*
* Se conecta al servidor y lee comandos del teclado: put ARCHIVO, get ARCHIVO y exit.
*
* @author Erwin Meza Vega <emezav@unicauca.edu.co>
* @author Edward Esteban Dávila Salazar
* @author Miguel Angel Perez Mera
* @copyright MIT License
*/
#include <arpa/inet.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/ip.h>
#include <unistd.h>

#include "protocol.h"
#include "split.h"
#include "fsutil.h"
#include "util.h"
#include "netutil.h"

/**
 * @brief Envia una solicitud al servidor
 *
 * @param s Socket conectado al servidor
 * @param operation Operacion (put, get, exit)
 * @param filename Nombre del archivo, o cadena vacia si la operacion no lo usa
 *
 * @return 1 si se envio completa, 0 si ocurre un error
 */
int send_request(int s, const char * operation, const char * filename);

/** @brief Indica que se recibio SIGINT o SIGTERM */
static volatile sig_atomic_t interrupted = 0;

/**
 * @brief Cuenta las palabras de una linea, separadas por espacios, tabuladores o saltos de linea
 *
 * @param line Linea a revisar
 *
 * @return Cantidad de palabras
 */
int count_words(const char * line);

/**
 * @brief Manejador de SIGINT y SIGTERM: marca que hay que terminar
 *
 * Tambien activa protocol_abort, para que una lectura o escritura interrumpida no se reintente.
 *
 * @param sig Numero de la senal
 */
void on_signal(int sig);

/**
 * @brief Lee una linea del teclado
 *
 * @param line Buffer que recibe la linea
 * @param size Capacidad del buffer
 *
 * @return 1 si leyo una linea, 0 si se acabo la entrada, -1 si la linea no cabia (se descarta)
 */
int read_line(char * line, size_t size);

/**
 * @brief Sube un archivo al servidor
 *
 * @param s Socket conectado al servidor
 * @param path Ruta del archivo local
 *
 * @return 1 si la conexion sigue util, 0 si se perdio
 */
int do_put(int s, const char * path);

/**
 * @brief Baja un archivo del servidor a la carpeta client_files
 *
 * @param s Socket conectado al servidor
 * @param client_dir Carpeta donde se guarda
 * @param name Nombre del archivo en el servidor
 *
 * @return 1 si la conexion sigue util, 0 si se perdio
 */
int do_get(int s, const char * client_dir, const char * name);

/**
 * @brief Pide al servidor la lista de los archivos del usuario y la muestra
 *
 * @param s Socket conectado al servidor
 *
 * @return 1 si la conexion sigue util, 0 si se perdio
 */
int do_list(int s);

int main(int argc, char * argv[]) {
	int s; // Conector
	const char * host = "127.0.0.1"; // Servidor por defecto: esta misma maquina
	char user[USER_MAX + 2] = ""; // Usuario con el que se identifica
	response login; // Respuesta del servidor al identificarse
	unsigned short port = DEFAULT_PORT; // Puerto por defecto
	long value;
	char * end;
	char base[PATH_MAX]; // Carpeta donde esta el programa
	char client_dir[PATH_MAX]; // Carpeta donde se guardan los archivos recibidos
	char line[PATH_MAX + 32]; // Linea escrita por el usuario
	struct sockaddr_in * addr; //Direccion IPv4 del socket del servidor
	split_list * cmd; // Linea dividida en palabras
	int alive; // 0 si se perdio la conexion con el servidor
	int got; // Resultado de leer una linea
	struct sigaction sa; // Manejador de SIGINT y SIGTERM

	if (argc > 4) {
		fprintf(stderr, "Uso: %s [ip] [puerto] [usuario]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	if (argc >= 2) {
		host = argv[1];
	}
	if (argc >= 3) {
		value = strtol(argv[2], &end, 10);
		if (*argv[2] == '\0' || *end != '\0' || value < 1 || value > 65535) {
			fprintf(stderr, "Puerto invalido: %s\n", argv[2]);
			exit(EXIT_FAILURE);
		}
		port = (unsigned short)value;
	}

	//SIGINT y SIGTERM terminan el cliente. Sin SA_RESTART, fgets y read se interrumpen
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = on_signal;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGTERM, &sa, NULL) == -1) {
		perror("sigaction");
		exit(EXIT_FAILURE);
	}

	//Un servidor que cierra de golpe no debe terminar el cliente (SIGPIPE)
	signal(SIGPIPE, SIG_IGN);

	//La carpeta client_files se crea junto al programa, si no existe
	if (!program_dir(base, sizeof(base))) {
		fprintf(stderr, "No se pudo determinar la carpeta del programa\n");
		exit(EXIT_FAILURE);
	}
	if (snprintf(client_dir, sizeof(client_dir), "%s/client_files", base) >= (int)sizeof(client_dir)) {
		fprintf(stderr, "La ruta del programa es demasiado larga\n");
		exit(EXIT_FAILURE);
	}
	if (!ensure_dir(client_dir)) {
		perror("No se pudo crear la carpeta client_files");
		exit(EXIT_FAILURE);
	}

	// 1. Obtener un conector IPv4
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	// 2. Conectar el socket a la direccion IPv4 del servidor
	addr = address_by_ip((char *)host, port);
	if (addr == NULL) {
		fprintf(stderr, "Direccion IPv4 invalida: %s\n", host);
		close(s);
		exit(EXIT_FAILURE);
	}
	printf("Conectando al servidor %s:%d...\n", host, port);
	if (connect(s, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) == -1) {
		perror("connect");
		free(addr);
		close(s);
		exit(EXIT_FAILURE);
	}
	free(addr);
	printf("Conectado!\n");

	//Lo primero es decir quien es: cada usuario tiene su propia carpeta en el servidor
	if (argc == 4) {
		snprintf(user, sizeof(user), "%s", argv[3]);
	} else {
		printf("Usuario: ");
		fflush(stdout);
		if (fgets(user, sizeof(user), stdin) == NULL) {
			close(s);
			exit(EXIT_FAILURE);
		}
		user[strcspn(user, "\r\n")] = 0;
	}
	if (!valid_user(user)) {
		fprintf(stderr, "Usuario no valido: de 1 a %d letras, numeros, _, - o .\n", USER_MAX);
		close(s);
		exit(EXIT_FAILURE);
	}
	if (!send_request(s, "user", user) || read_all(s, &login, sizeof(login)) != (ssize_t)sizeof(login)) {
		fprintf(stderr, "Se perdio la conexion con el servidor\n");
		close(s);
		exit(EXIT_FAILURE);
	}
	login.message[sizeof(login.message) - 1] = 0;
	if (login.code != TRANSFER_AUTHORIZED) {
		fprintf(stderr, "Servidor: %s\n", login.message);
		close(s);
		exit(EXIT_FAILURE);
	}
	printf("Usuario %s. Comandos: put ARCHIVO | get ARCHIVO | list | exit\n", user);

	// 3. Comunicacion: leer un comando, enviarlo y esperar la respuesta
	while (1) {
		printf("> ");
		fflush(stdout);

		//Ctrl-D (fin de entrada) equivale a exit
		got = read_line(line, sizeof(line));
		if (got == 0) {
			send_request(s, "exit", "");
			break;
		}
		if (got < 0) {
			printf("La linea es demasiado larga\n");
			continue;
		}

		//split() guarda hasta MAX_PARTS palabras: una linea con mas se rechaza antes
		if (count_words(line) >= MAX_PARTS) {
			printf("La linea tiene demasiadas palabras\n");
			continue;
		}

		cmd = split(line, NULL);
		if (cmd == NULL) {
			printf("No hay memoria para interpretar el comando\n");
			break;
		}
		if (cmd->count == 0) {
			free_split_list(cmd);
			continue; //Linea vacia
		}
		lcase(cmd->parts[0]);

		if (equals(cmd->parts[0], "exit")) {
			send_request(s, "exit", "");
			free_split_list(cmd);
			break;
		}

		if (equals(cmd->parts[0], "list")) {
			alive = do_list(s);
			free_split_list(cmd);
			if (!alive) {
				break;
			}
			continue;
		}

		if (equals(cmd->parts[0], "put") || equals(cmd->parts[0], "get")) {
			if (cmd->count < 2) {
				printf("Uso: %s ARCHIVO\n", cmd->parts[0]);
			} else if (strlen(cmd->parts[1]) >= NAME_MAX) {
				printf("El nombre del archivo es demasiado largo\n");
			} else if (equals(cmd->parts[0], "put")) {
				alive = do_put(s, cmd->parts[1]);
			} else {
				alive = do_get(s, client_dir, cmd->parts[1]);
			}
			if (!alive) {
				free_split_list(cmd);
				break;
			}
		} else {
			printf("Comando desconocido. Use: put ARCHIVO | get ARCHIVO | list | exit\n");
		}

		free_split_list(cmd);
	}

	if (interrupted) {
		printf("\nInterrumpido\n");
	}

	// 4. Cerrar la conexion con el servidor
	printf("Cerrando conexion...\n");
	close(s);

	exit(EXIT_SUCCESS);
}

int send_request(int s, const char * operation, const char * filename) {
	request req;

	memset(&req, 0, sizeof(req));
	strncpy(req.operation, operation, sizeof(req.operation) - 1);
	strncpy(req.filename, filename, sizeof(req.filename) - 1);

	return write_all(s, &req, sizeof(req)) == (ssize_t)sizeof(req);
}

int count_words(const char * line) {
	int count = 0;
	int in_word = 0;

	for (; *line != '\0'; line++) {
		if (*line == ' ' || *line == '\t' || *line == '\r' || *line == '\n') {
			in_word = 0;
		} else if (!in_word) {
			in_word = 1;
			count++;
		}
	}

	return count;
}

int read_line(char * line, size_t size) {
	int c;

	if (fgets(line, size, stdin) == NULL) {
		return 0;
	}
	if (strchr(line, '\n') != NULL || feof(stdin)) {
		return 1;
	}

	//no cupo la linea entera: se descarta lo que falta para que no se lea como otro comando
	while ((c = getchar()) != '\n' && c != EOF);
	return -1;
}

int do_put(int s, const char * path) {
	struct stat st;
	const char * name;
	response res;

	if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
		printf("%s no existe o no es un archivo\n", path);
		return 1;
	}

	//al servidor le llega solo el nombre, sin la ruta
	name = strrchr(path, '/');
	name = (name == NULL) ? path : name + 1;

	if (!send_request(s, "put", name) || read_all(s, &res, sizeof(res)) != (ssize_t)sizeof(res)) {
		printf("Se perdio la conexion con el servidor\n");
		return 0;
	}
	res.message[sizeof(res.message) - 1] = '\0';
	printf("Servidor: %s\n", res.message);
	if (res.code != TRANSFER_AUTHORIZED) {
		return 1;
	}

	//autorizado: se manda el archivo y se espera la confirmacion
	if (!send_file(s, (char *)path) || read_all(s, &res, sizeof(res)) != (ssize_t)sizeof(res)) {
		printf("Fallo el envio, se cierra la conexion\n");
		return 0;
	}
	res.message[sizeof(res.message) - 1] = '\0';
	printf("Servidor: %s\n", res.message);
	return 1;
}

int do_get(int s, const char * client_dir, const char * name) {
	int r;

	if (!send_request(s, "get", name)) {
		printf("Se perdio la conexion con el servidor\n");
		return 0;
	}

	r = receive_file(s, (char *)client_dir, NULL);
	if (r == 1) {
		printf("Servidor: archivo guardado en client_files/%s\n", name);
	} else if (r == -1) {
		printf("Servidor: el archivo %s no existe\n", name);
	} else {
		printf("Fallo la descarga, se cierra la conexion\n");
		return 0;
	}
	return 1;
}

void on_signal(int sig) {
	(void)sig;
	interrupted = 1;
	protocol_abort = 1;
}

int do_list(int s) {
	file_info info;
	int count = 0;

	if (!send_request(s, "list", "")) {
		printf("Se perdio la conexion con el servidor\n");
		return 0;
	}

	//un file_info por archivo; el que trae size -1 cierra la lista
	while (1) {
		if (read_all(s, &info, sizeof(info)) != (ssize_t)sizeof(info)) {
			printf("Se perdio la conexion con el servidor\n");
			return 0;
		}
		if (info.size < 0) {
			break;
		}
		info.filename[sizeof(info.filename) - 1] = '\0';
		printf("  %-30s %lld bytes\n", info.filename, (long long)info.size);
		count++;
	}

	if (count == 0) {
		printf("No hay archivos\n");
	}
	return 1;
}
