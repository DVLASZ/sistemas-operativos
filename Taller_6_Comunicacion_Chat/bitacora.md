# Bitácora — Taller: chat con sockets TCP e hilos POSIX

**Autores:** Edward Esteban Dávila Salazar, Miguel Angel Perez Mera

**Entorno de verificación:** Debian 13 (trixie), `gcc 14.2.0`, Doxygen 1.9.8,
máquina virtual de VirtualBox del laboratorio de la materia (4 núcleos).

La guía entrega el chat ya escrito: no hay puntos de código que
completar. El trabajo se divide en dos partes:

1. **Primera parte:** poner el código en funcionamiento, comentarlo,
   escribir `Makefile`, `Doxyfile` y `README`, ejecutarlo con tres terminales
   y responder los **puntos para discutir en clase**.
2. **Segunda parte:** el **ejercicio propuesto** (apodos), que modifica el
   código, junto con dos cambios más: mostrar la **dirección del cliente** en
   la consola del servidor y corregir el defecto de **`SIGPIPE`** que
   apareció en la primera parte.

Las pruebas se hicieron dentro de la máquina virtual, con el servidor y
los clientes en la misma máquina (`127.0.0.1`, puerto 8080). Las
"terminales" de las pruebas son procesos en segundo plano cuya entrada
estándar se alimenta desde un guion (con pausas de `sleep` y `echo`); lo que
cada uno imprime se guardó en un archivo, y es lo que se muestra abajo.

## Qué se hizo sobre el código de la guía

- Se agregaron **comentarios básicos** a lo largo de `chat_server.c` y
  `chat_client.c`: qué es cada variable, qué hace cada llamada de sockets e
  hilos y por qué. Las cabeceras ya traían su documentación Doxygen.
- Se agregaron los nombres de los autores a los encabezados de los
  archivos.
- En la primera parte se comprobó que **la lógica no cambió**: al quitarles
  los comentarios, `chat_server.c` (142 líneas de código) y `chat_client.c`
  (97 líneas) resultaban idénticos a los originales de la guía. La segunda
  parte sí modifica el código (ver más abajo); al comentarlo por completo se
  volvió a comprobar que solo cambian los comentarios entre una versión y
  otra.
- Se escribió el `Makefile`, el `Doxyfile` y el `README`.

## Compilación

```bash
make
```

```
gcc -std=c11 -Wall -Wextra -pthread -o chat_server src/chat_server.c
gcc -std=c11 -Wall -Wextra -pthread -o chat_client src/chat_client.c
```

Compila sin ninguna advertencia. Los comandos que indica la guía
(`gcc -std=c11 -Wall -Wextra -pthread src/chat_server.c -o chat_server` y su
equivalente para el cliente) también compilan sin advertencias.
`doxygen Doxyfile` tampoco informa advertencias.

## Ejecución con tres terminales (código de la guía, primera parte)

Servidor en la terminal 1, cliente A en la 2 y cliente B en la 3. El
cliente A escribe `Hola a todos`; después B escribe `Hola A, aqui B`. Esta
ejecución es del código **tal como lo entrega la guía**, sin apodos; la
ejecución con apodos está en la segunda parte.

```
--- Terminal 1: servidor ---
Servidor de chat en el puerto 8080
[servidor] Cliente 4 (127.0.0.1) se unió al chat
[servidor] Cliente 5 (127.0.0.1) se unió al chat
[4] Hola a todos
[5] Hola A, aqui B
[servidor] Cliente 4 (127.0.0.1) salió del chat
[servidor] Cliente 5 (127.0.0.1) salió del chat

--- Terminal 2: cliente A ---
Conectado al chat en 127.0.0.1:8080
Escribe mensajes (Ctrl-D para salir):
[servidor] Cliente 5 (127.0.0.1) se unió al chat
[5] Hola A, aqui B

--- Terminal 3: cliente B ---
Conectado al chat en 127.0.0.1:8080
Escribe mensajes (Ctrl-D para salir):
[4] Hola a todos
[servidor] Cliente 4 (127.0.0.1) salió del chat
```

Se ve lo que describe la guía: el mensaje de A llega a B prefijado con el
número de descriptor de A (`[4]`), el servidor imprime todo lo que pasa en
su propia terminal, y cada cliente se entera de quién entra y quién sale.
El emisor de un mensaje no lo recibe de vuelta.

Los números de descriptor (4 y 5) son los que dio el sistema en esta
ejecución: 0, 1 y 2 son la entrada, la salida y el error estándar, y el 3 es
el socket de escucha, así que el primer cliente aceptado recibe el 4. La
guía muestra 5 y 6 porque su servidor tenía otro descriptor abierto.

## Puntos para discutir en clase

### 1. ¿Por qué `client_conn_t` va en el heap y no en la pila del hilo principal? ¿Qué condición de carrera se evita?

El hilo principal repite en un ciclo `accept → crear hilo`. Sus variables
locales (`client_fd`, `client_addr`) son **una sola copia** que `accept`
sobrescribe en cada vuelta. `pthread_create` regresa de inmediato, antes de
que el hilo nuevo haya tenido tiempo de leer su argumento. Si el argumento
fuera la dirección de esa variable local:

- el hilo principal volvería a `accept` y, al llegar el siguiente cliente,
  **escribiría** `client_addr` mientras el hilo anterior aún no lo ha
  **leído**;
- el hilo podría leer los datos del cliente equivocado: dos hilos
  terminarían atendiendo el mismo cliente y el primero se quedaría sin
  servicio.

Es una condición de carrera clásica (un escritor y un lector sin
sincronización sobre el mismo dato). Con `malloc` cada conexión tiene **su
propia copia** de `fd` y de la dirección: nadie más la toca. El hilo la
libera con `free(conn)` apenas copia lo que necesita (la IP en `peer`), lo
que además evita una fuga de memoria.

### 2. ¿Qué ocurre si se llama a `send()` dentro de la sección crítica? ¿Por qué el patrón "copiar y liberar" lo soluciona?

`send()` puede **bloquearse**: si el cliente destinatario no está leyendo
(se congeló, es lento o su red está saturada), los búfers del socket se
llenan y `send()` espera hasta que haya sitio, y eso puede tardar
indefinidamente. Si en ese momento el hilo sostiene el mutex de la lista,
todo lo que necesite ese mutex queda detenido detrás de él:

- el hilo principal no puede registrar clientes nuevos (`client_list_add`);
- ningún cliente puede salir (`client_list_remove`);
- ningún otro hilo puede difundir (`broadcast`).

Es decir, **un solo cliente lento congelaría el chat entero**, porque el
mutex se retiene durante una operación de red cuya duración no controla el
servidor.

El patrón de la guía separa las dos cosas: bajo el mutex solo se **copia**
la lista de descriptores a un arreglo local (`snapshot`), un trabajo de unos
pocos nanosegundos que no puede bloquearse; se libera el mutex y recién
entonces se envía a cada destinatario. Si un `send()` se bloquea, solo se
bloquea el hilo que lo hizo; la lista sigue disponible para todos los
demás.

El patrón tiene un costo que conviene conocer: la copia puede quedar
desactualizada. Un cliente puede salir (y su descriptor cerrarse) entre la
copia y el envío; en ese caso `send()` falla con error. Es un compromiso
aceptable frente al bloqueo general, pero ver las observaciones al final.

Esto lo razoné a partir del código; no lo reproduje con una variante que
envíe dentro del mutex.

### 3. ¿Por qué se usa `pthread_detach()` en lugar de `pthread_join()` para los hilos de cliente? ¿En qué caso sería preferible `pthread_join()`?

`pthread_join()` **bloquea** al hilo que lo llama hasta que el otro termina.
El hilo principal del servidor no puede detenerse a esperar a un cliente: su
trabajo es volver a `accept()` para atender al siguiente. Además, a nadie le
interesa el valor que retorna un hilo de cliente (siempre `NULL`).

Un hilo que termina sin estar separado ni unido queda como un "zombi" que
conserva sus recursos hasta que alguien le haga `join`. En un servidor que
vive días y atiende miles de clientes eso sería una fuga continua.
`pthread_detach()` le dice al sistema: "libera todo cuando este hilo
termine, nadie va a preguntar por él".

`pthread_join()` es preferible cuando **sí** hace falta esperar al hilo o
recoger su resultado:

- un hilo trabajador devuelve un valor calculado que el creador necesita;
- el creador debe asegurarse de que el hilo terminó antes de liberar o
  cerrar algo que ese hilo usa (por ejemplo, un apagado ordenado del servidor
  que espera a todos los hilos de clientes antes de destruir la lista y su
  mutex);
- se lanza un número fijo de hilos y hay que combinar sus resultados.

### 4. ¿Por qué `shutdown(sock_fd, SHUT_RDWR)` no despierta el `select()` que vigila `stdin`? ¿Qué papel cumple el timeout?

`select()` solo se despierta por eventos de **los descriptores que se le
pasaron** (aquí, únicamente `STDIN_FILENO`), por el vencimiento del timeout
o por una señal. `shutdown()` actúa sobre el **socket** (`sock_fd`), que es
otro descriptor distinto: el estado de la entrada estándar no cambia, así
que `select()` no tiene motivo para retornar.

Por eso la guía usa el timeout: es la forma de que el hilo emisor se
despierte cada segundo aunque nadie escriba, y consulte la variable
`running`. Cuando el receptor detecta que el servidor se fue, pone
`running = 0`, y el emisor lo nota la próxima vez que vence el timeout.

Se midió: se tumbó el servidor con un cliente ocioso conectado, en seis
ocasiones y en instantes distintos dentro del ciclo del `select()`. El
cliente terminó entre **0.24 s y 0.98 s** después de la caída (0.83, 0.66,
0.49, 0.33, 0.24 y 0.98 s), siempre por debajo de 1 s, y mostró
`Conexión cerrada por el servidor.`. El valor depende de en qué punto del
segundo caiga el servidor, porque el timeout se reinicia en cada vuelta.

### 5. El emisor usa `select()` con timeout de 1 segundo sobre `stdin`. ¿Qué consecuencia tiene para la experiencia del usuario? ¿Cómo se podría reducir o eliminar ese retardo?

Qué **no** retrasa: escribir. Cuando el usuario teclea una línea, `select()`
retorna al instante porque `stdin` quedó listo; el timeout solo importa
cuando no hay nada escrito.

Qué sí retrasa: **enterarse de que el servidor se cayó**. Hasta que vence el
timeout el programa sigue pareciendo vivo (hasta 1 s, como se midió arriba).
Y hay un efecto menos visible: si el usuario escribe una línea en esa
ventana, `fgets()` la lee pero el programa descubre después que
`running == 0` y la descarta sin avisar, así que el mensaje se pierde.

Formas de reducirlo o eliminarlo:

- **Un solo hilo con `select()` o `poll()` sobre `stdin` y el socket a la
  vez.** Cuando el servidor cierra, el socket queda "listo para leer" (con 0
  bytes) y el `select()` se despierta de inmediato, sin timeout y sin hilo
  receptor.
- **Tubería de despertar (self-pipe) o `eventfd`.** Se agrega el extremo de
  lectura de una tubería al conjunto de `select()`; el hilo receptor escribe
  un byte en ella al terminar, y el emisor se despierta al instante conservando
  la estructura de dos hilos.
- **Interrumpir el `select()` con una señal** dirigida al hilo emisor
  (`pthread_kill`), con un manejador sin `SA_RESTART`, como en el taller de
  señales: `select()` retorna `-1` con `EINTR`.
- **Acortar el timeout** (por ejemplo 100 ms): reduce el retardo, pero a costa
  de despertar el programa diez veces por segundo sin necesidad.

La mejor es la primera o la segunda: eliminan el retardo sin sondeo.

### 6. ¿Qué sucede si un cliente envía un mensaje muy largo que supera `BUFSIZE`? ¿Cómo se podría manejar correctamente?

Se probó con una línea de 1300 caracteres. Qué pasa:

- El cliente la lee con `fgets(buf, BUFSIZE, stdin)`, que entrega **como
  máximo 511 caracteres** por llamada, y envía cada trozo por separado.
- El servidor recibe con `recv(fd, buf, BUFSIZE - 1, 0)` y trata cada
  llamada como si fuera un mensaje completo.

Resultado medido (servidor y cliente B, el mismo):

```
trozo 1: prefijo [5] + 511 caracteres
trozo 2: prefijo [5] + 511 caracteres
trozo 3: prefijo [5] + 278 caracteres
```

Un solo mensaje llegó como **tres mensajes**, cada uno con su propio
prefijo `[5]`. En la consola del servidor los tres trozos salen pegados en
una sola línea, porque solo el último trae el salto de línea.

La causa de fondo es que **TCP es un flujo de bytes sin fronteras de
mensaje**: lo que se envía con varios `send()` puede llegar en un `recv()` o
en varios, y lo que se envía con uno solo puede partirse. Por eso el
problema no se limita a mensajes largos: dos mensajes cortos podrían llegar
juntos en un solo `recv()`, y una frase cualquiera podría cortarse a medias.
Además un trozo puede partir un carácter UTF-8 de varios bytes (como la `ó`)
y mostrarlo corrupto.

Cómo manejarlo correctamente: definir un **protocolo con fronteras**.

- **Mensajes terminados en `\n`** (lo más sencillo aquí): cada hilo de cliente
  mantiene un búfer propio al que acumula lo que va llegando y solo procesa
  cuando encuentra un `\n`; lo que sobra se queda para la siguiente lectura.
- **Prefijo de longitud**: cada mensaje empieza con su tamaño, y el receptor
  lee exactamente esa cantidad de bytes.
- En ambos casos, fijar un **tamaño máximo de mensaje** y definir qué hacer
  si se excede (cortar y avisar, o rechazar y cerrar la conexión), en vez de
  dejar que el límite lo imponga el tamaño del búfer.
- Del lado del emisor, comprobar el valor que devuelve `send()`, porque puede
  enviar menos bytes de los pedidos y hay que repetir hasta completar.

## Segunda parte: ejercicio de los apodos, dirección del cliente y `SIGPIPE`

### Qué se pidió

1. **Apodos** (ejercicio propuesto de la guía): el cliente envía su apodo
   como **primera línea** al conectarse; el servidor la lee en el hilo del
   cliente **antes** de entrar al ciclo `recv`, la guarda junto al
   descriptor en la lista de clientes, y los mensajes difundidos usan el
   apodo en lugar del número de descriptor (`[alice] Hola.`).
2. **Dirección del cliente en el servidor:** la consola del servidor debe
   mostrar de dónde viene cada cliente.
3. **`SIGPIPE`:** corregir el defecto que la primera parte dejó
   reproducido (más abajo).

### Cambios en el código

| Archivo | Cambio |
| --- | --- |
| `chat.h` | nueva constante `NICK_SIZE` (32, con el terminador) |
| `chat_server.h` | `client_list_t` gana `nicknames[MAX_CLIENTS][NICK_SIZE]`; nuevas funciones `client_list_set_nickname()`, `client_list_get_nickname()` y `read_line()` |
| `chat_server.c` | `client_thread()` lee el apodo antes del ciclo, arma `ip:puerto` para la consola y usa el apodo en los mensajes; `broadcast()` envía con `MSG_NOSIGNAL` |
| `chat_client.c` | pide el apodo y lo envía como primera línea, antes de crear el hilo receptor; su `send()` usa `MSG_NOSIGNAL` |

**Dónde se guarda el apodo.** La guía sugiere ampliar `client_conn_t` con
`char nickname[32]`, pero esa estructura solo sirve para pasarle datos al
hilo recién creado y el hilo la libera con `free()` apenas arranca: no
puede ser el lugar donde viva el apodo mientras el cliente esté conectado.
La propia guía dice también que se almacene "junto al descriptor en la lista
de clientes", así que el apodo vive en `client_list_t`, en la misma posición
que su descriptor. Como otros hilos pueden leer esa lista durante un
`broadcast()`, tanto escribir como leer el apodo se hace **bajo el mismo
mutex** que protege la lista.

**Cómo se lee el apodo.** El servidor usa `read_line()`, que lee **un byte por
vez** hasta el salto de línea. No se usó un `recv()` normal porque TCP es un
flujo de bytes: si el cliente enviara el apodo y su primer mensaje juntos,
un `recv()` los traería pegados y el apodo saldría con el mensaje dentro.
`read_line()` no consume nada más allá de su línea, así que lo que sigue
queda intacto para el ciclo de mensajes. Un apodo vacío recibe uno
automático (`cliente<descriptor>`), uno de más de 31 caracteres se trunca, y
un cliente que se va sin presentarse se quita de la lista sin anuncios.

**Dirección del cliente.** `client_thread()` arma `ip:puerto` con
`inet_ntop()` y `ntohs()` y lo muestra en la consola del servidor al entrar,
en cada mensaje y al salir. A los demás clientes les llega el apodo y, en los
avisos de entrada y salida, la IP (como en la versión de la guía), no el
puerto.

### Ejecución con apodos

Servidor y tres clientes: `alice` y `bob` con apodo, y un tercero que pulsa
Enter sin escribir apodo. En las salidas de los clientes el aviso `Escribe tu
apodo:` queda pegado al siguiente texto solo porque aquí la entrada viene de
un guion; en una terminal el Enter del usuario deja el salto de línea.

```
--- servidor (con la direccion de cada cliente) ---
Servidor de chat en el puerto 8080
[servidor] alice (127.0.0.1:56896) se unió al chat
[servidor] bob (127.0.0.1:56904) se unió al chat
[servidor] cliente6 (127.0.0.1:56916) se unió al chat
[alice 127.0.0.1:56896] Hola a todos
[bob 127.0.0.1:56904] Hola alice, soy bob
[cliente6 127.0.0.1:56916] yo no puse apodo
[servidor] alice (127.0.0.1:56896) salió del chat
[servidor] bob (127.0.0.1:56904) salió del chat
[servidor] cliente6 (127.0.0.1:56916) salió del chat

--- cliente alice ---
Conectado al chat en 127.0.0.1:8080
Escribe tu apodo: Escribe mensajes (Ctrl-D para salir):
[servidor] bob (127.0.0.1) se unió al chat
[servidor] cliente6 (127.0.0.1) se unió al chat
[bob] Hola alice, soy bob
[cliente6] yo no puse apodo

--- cliente bob ---
Conectado al chat en 127.0.0.1:8080
Escribe tu apodo: Escribe mensajes (Ctrl-D para salir):
[servidor] cliente6 (127.0.0.1) se unió al chat
[alice] Hola a todos
[cliente6] yo no puse apodo
[servidor] alice (127.0.0.1) salió del chat
```

Se ve lo pedido: los mensajes se difunden con el apodo (`[alice] Hola a
todos`), la consola del servidor muestra la dirección y el puerto de cada
cliente, y el que no puso apodo quedó como `cliente6` (el 6 es su
descriptor).

### Casos límite del apodo

```
[servidor] pepe (127.0.0.1:58402) se unió al chat
[pepe 127.0.0.1:58402] primer mensaje pegado al apodo
[servidor] abcdefghijklmnopqrstuvwxyz01234 (127.0.0.1:58410) se unió al chat
[servidor] pepe (127.0.0.1:58402) salió del chat
[abcdefghijklmnopqrstuvwxyz01234 127.0.0.1:58410] mensaje con apodo largo
[servidor] abcdefghijklmnopqrstuvwxyz01234 (127.0.0.1:58410) salió del chat
```

- `pepe` envió `pepe\nprimer mensaje pegado al apodo\n` en **un solo
  envío** TCP: el apodo salió limpio y el mensaje llegó entero, sin que el
  apodo se tragara parte de él.
- Un apodo de 45 caracteres quedó truncado a 31.
- Un cliente que se conectó y se fue sin escribir nada no produjo ningún
  anuncio ni dejó nada en la lista.

### `SIGPIPE`: el defecto de la primera parte, corregido

La causa (ver más abajo, en las observaciones) es que `send()` a un socket
cuyo cliente ya se fue termina generando `SIGPIPE`, y su acción por defecto
mata al servidor. El cambio es pasar `MSG_NOSIGNAL` en los `send()` de
`broadcast()` (servidor) y del emisor (cliente): con esa opción, un envío a
un cliente que ya cerró falla con `EPIPE` en lugar de enviar la señal. No
hace falta tratar el error: el hilo del cliente que se fue limpia su propia
entrada al detectar el cierre en su `recv()`.

La misma prueba que mató al servidor de la guía en 5 de 5 rondas (un cliente
que escribe una línea cada milisegundo y 150 clientes que se conectan y se
van a los 4 ms sin leer nada), repetida con el servidor corregido:

```
ronda 1: el servidor sigue vivo
ronda 2: el servidor sigue vivo
ronda 3: el servidor sigue vivo
ronda 4: el servidor sigue vivo
ronda 5: el servidor sigue vivo
```

## Observaciones sobre el código base

Mientras se respondían los puntos aparecieron cosas del código de la guía
que vale la pena dejar anotadas. La de `SIGPIPE` se corrigió en la segunda
parte; las demás quedan como observaciones.

### El servidor muere con `SIGPIPE` (reproducido; corregido en la segunda parte)

El servidor no ignora `SIGPIPE` ni usa `MSG_NOSIGNAL` en `send()`. Cuando
`broadcast()` escribe en un socket cuyo cliente ya cerró la conexión, la
primera escritura "funciona" y el otro extremo responde con un `RST`; la
siguiente escritura falla con `EPIPE` y el sistema envía `SIGPIPE`, cuya
acción por defecto es **terminar el proceso**. Es el mismo mecanismo que el
punto 8 del taller de señales.

Prueba: un cliente que escribe una línea cada milisegundo, y 150 clientes que
se conectan y se van a los 4 ms sin leer nada.

```
ronda 1: EL SERVIDOR MURIO, codigo de salida 141 (141 = 128 + 13 = SIGPIPE)
ronda 2: EL SERVIDOR MURIO, codigo de salida 141 (141 = 128 + 13 = SIGPIPE)
ronda 3: EL SERVIDOR MURIO, codigo de salida 141 (141 = 128 + 13 = SIGPIPE)
ronda 4: EL SERVIDOR MURIO, codigo de salida 141 (141 = 128 + 13 = SIGPIPE)
ronda 5: EL SERVIDOR MURIO, codigo de salida 141 (141 = 128 + 13 = SIGPIPE)
```

En las cinco rondas el servidor de la guía murió, y con él todas las
conexiones. En uso normal es raro (hace falta enviar justo mientras un
cliente se desconecta), pero un chat real lo encontraría pronto. Se corrigió
con `MSG_NOSIGNAL` (ver la segunda parte): con el servidor corregido, la
misma prueba dejó el servidor vivo las cinco veces.

### Otras observaciones (por lectura del código, sin reproducir)

- **Lista llena.** Con 32 clientes (`MAX_CLIENTS`), `client_list_add()`
  no encuentra posición libre y no avisa: el cliente número 33 se conecta y
  su hilo funciona, pero nunca recibe lo que difunden los demás.
- **Descriptor reutilizado.** Entre la copia de `snapshot` y el `send()`, un
  cliente puede salir y su descriptor ser reasignado a una conexión nueva; el
  mensaje iría entonces al cliente equivocado.
- **`send()` sin comprobar** en `broadcast()`: un fallo o un envío parcial
  pasan desapercibidos. Con `MSG_NOSIGNAL` un fallo ya no mata al servidor,
  pero sigue sin informarse.
- **`pthread_create` informado con `perror`**: las funciones de `pthread` no
  usan `errno` (devuelven el código de error), así que `perror` imprimiría un
  mensaje que no corresponde.

## Lista de comprobación

Primera parte:

- [x] Código de la guía compilando sin advertencias (`make` y los comandos
  de la guía).
- [x] Comentarios básicos a lo largo del código, sin cambiar la lógica
  (verificado contra los originales).
- [x] `Makefile`, `Doxyfile` y `README`; Doxygen sin advertencias.
- [x] Ejecución con tres terminales: servidor, cliente A y cliente B.
- [x] Los seis puntos para discutir respondidos, con medidas donde se pudo.
- [x] Observaciones sobre el código base, con el defecto de `SIGPIPE`
  reproducido.

Segunda parte:

- [x] Ejercicio propuesto de los apodos: primera línea del cliente, lectura
  en el hilo antes del ciclo `recv`, almacenamiento junto al descriptor bajo
  el mutex, mensajes con el apodo.
- [x] Dirección del cliente (`ip:puerto`) en la consola del servidor.
- [x] Casos límite del apodo probados (vacío, largo, pegado al primer
  mensaje, cliente que se va sin presentarse).
- [x] `SIGPIPE` corregido con `MSG_NOSIGNAL` y comprobado con la misma
  prueba que lo reprodujo.
- [x] Compila sin advertencias y Doxygen sin advertencias con el código
  final.
