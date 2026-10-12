# Bitácora — Taller: señales en un servidor concurrente

**Autores:** Edward Esteban Dávila Salazar, Miguel Angel Perez Mera

**Entorno de verificación:** Debian 13 (trixie), `gcc 14.2.0`, Doxygen 1.9.8,
máquina virtual de VirtualBox del laboratorio de la materia (4 núcleos).

Todas las pruebas se hicieron dentro de la máquina virtual, con el
servidor y los clientes en la misma máquina (`127.0.0.1`, puerto 5500).
Las señales se enviaron con `kill -INT` / `kill -ALRM` / `kill -KILL`
desde el guion de prueba; `kill -INT` es lo mismo que recibe el proceso al
pulsar Ctrl-C, porque la terminal le entrega `SIGINT` al grupo de procesos
en primer plano.

## Mapa del enunciado

| Punto | Qué pide | Dónde se resuelve |
| --- | --- | --- |
| Preparación | compilar y arrancar el servidor | sección *Preparación* |
| 1 | servidor original + Ctrl-C: qué presenta y qué pasa con el cliente | *Punto 1* |
| 2 | bandera `volatile sig_atomic_t`, manejador de `SIGINT`, `sigaction` sin `SA_RESTART` | *Puntos 2 y 3* |
| 3 | ciclo que termina con la bandera, `EINTR`, cerrar el socket, presentar contadores | *Puntos 2 y 3* |
| 4 | proteger los contadores y justificar el mecanismo | *Punto 4* |
| 5 | `SIGALRM`, `alarm` y el informe periódico | *Punto 5* |
| 6 | `kill -ALRM/-INT/-KILL` y el intento de capturar `SIGKILL` | *Punto 6* |
| 7 | valores de retorno, `perror` y un fallo provocado | *Punto 7* |
| 8 | efecto de `signal(SIGPIPE, SIG_IGN)` | *Punto 8* |

La bitácora sigue ese mismo orden. Las respuestas que la entrega exige
explícitamente son las de los puntos 1, 4, 6, 7 y 8 y la salida de un
informe periódico (en el punto 5); aquí además se documentan los demás
puntos.

## Preparación

El directorio `src/` trae el servidor, el cliente de carga, el `Makefile`
y `carga.sh` de la práctica anterior. Se compila y se arranca así:

```bash
cd src
make
./servidor_tablero rwlock
```

```
gcc -Wall -Wextra -O2 -o servidor_tablero servidor_tablero.c -pthread
gcc -Wall -Wextra -O2 -o cliente_tablero cliente_tablero.c
```

Compila sin ninguna advertencia, tanto el servidor original como el
modificado. Al arrancar, el servidor imprime
`Tablero de 64 casillas, politica rwlock, escuchando en el puerto 5500`.

## Qué se cambió y qué no

Solo se modificó `src/servidor_tablero.c`. `cliente_tablero.c`,
`carga.sh` y el `Makefile` quedaron como los entregó el docente (se
comprobó byte a byte contra el zip). Compila sin advertencias con
`gcc -Wall -Wextra -O2` (lo hace el `Makefile`) y `doxygen Doxyfile` no
informa advertencias.

Resumen de los cambios en el servidor:

| Qué | Dónde |
| --- | --- |
| Banderas `terminar` e `informar` (`volatile sig_atomic_t`) | variables estáticas |
| Manejadores `al_interrumpir` y `al_sonar_alarma`, que solo asignan la bandera | funciones estáticas |
| `instalar_manejador()`: `sigaction` con `sa_flags = 0` | usada desde `main` |
| Ciclo `while (!terminar)` con `accept` que distingue `EINTR` | `main` |
| Informe periódico `presentar_estado()` y cierre ordenado con `presentar_contadores()` | `main` |
| Contadores `lecturas_atendidas` y `escrituras_atendidas` (`atomic_long`) | `atender_cliente` |
| Los hilos de los clientes nacen con `SIGINT` y `SIGALRM` bloqueadas | `crear_hilo_cliente()` |
| Comprobación de errores de las llamadas agregadas | ver punto 7 |

## Punto 1 — el servidor original, detenido con Ctrl-C

Se compiló el servidor sin modificaciones, se conectaron dos clientes (un
lector que pedía 300 lecturas y un `nc` ocioso, sin enviar nada) y se le
envió `SIGINT`. Como la prueba corrió desde un guion sin terminal, donde
los procesos en segundo plano heredan `SIGINT` ignorada, el servidor se
lanzó restableciendo esa señal a su acción por defecto, que es la que
tiene en una terminal interactiva:

```
servidor: codigo de salida 130 (128+2 = terminado por SIGINT)

--- stdout/stderr del servidor ---
Tablero de 64 casillas, politica rwlock, escuchando en el puerto 5500

--- cliente lector ---
La conexion se interrumpio en la repeticion 19
18 lecturas, 0 incoherentes, 1.279 segundos

--- nc ocioso ---
nc ocioso termino con codigo 0

--- socket de escucha tras la muerte ---
(el puerto 5500 ya no escucha)
```

**Qué presenta el servidor al terminar:** nada. Lo último que escribió
fue el mensaje de arranque. No informa cuántas lecturas ni escrituras
atendió, porque `SIGINT` tiene como acción por defecto terminar el
proceso en el acto: ninguna línea de `main` se ejecuta después. El código
de salida es 130 (128 + 2), que es como el shell informa "murió por la
señal 2".

**Qué ocurre con el cliente conectado:** su conexión se corta de golpe.
El lector, que iba por la lectura 19 de 300, recibe fin de archivo en
`read` y termina con "La conexión se interrumpió en la repetición 19".
El `nc` ocioso también termina, porque su `read` devuelve 0.

El servidor tampoco cerró el socket de escucha: el programa nunca llega a
la línea `close(socket_escucha)`. El puerto dejó de escuchar solo porque
el núcleo cierra todos los descriptores de un proceso cuando muere; el
programa no tuvo ninguna participación, ni oportunidad de avisar o de
limpiar nada.

## Puntos 2 y 3 — manejador de `SIGINT` y ciclo que termina

```c
static volatile sig_atomic_t terminar = 0;

static void al_interrumpir(int senal) {
    (void) senal;
    terminar = 1;
}
```

El manejador no hace nada más que activar la bandera, que es lo único
seguro dentro de un manejador: se ejecuta en medio de lo que el proceso
estuviera haciendo, así que `printf` (que no es reentrante) o la toma de un
candado (que el hilo interrumpido podría tener) son un riesgo de bloqueo.
La bandera es `volatile sig_atomic_t` para que el compilador no la guarde
en un registro dentro del ciclo (no vería el cambio) y para que la
asignación sea indivisible.

La instalación se hace en `main`, antes de crear el socket y de aceptar la
primera conexión, con `sigaction` y `sa_flags = 0` (sin `SA_RESTART`):

```c
accion.sa_handler = manejador;
sigemptyset(&accion.sa_mask);
accion.sa_flags = 0;
sigaction(senal, &accion, NULL)
```

Sin `SA_RESTART`, cuando llega la señal mientras el hilo principal espera
en `accept`, esa llamada retorna `-1` con `errno == EINTR` en lugar de
reanudarse, y el ciclo recupera el control para mirar la bandera. Con
`SA_RESTART` el sistema reanudaría `accept` y el servidor parecería no
reaccionar a Ctrl-C.

El ciclo queda así:

```c
while (!terminar) {
    if (informar) { informar = 0; presentar_estado(); alarm(INTERVALO_INFORME); }

    socket_cliente = accept(socket_escucha, NULL, NULL);
    if (socket_cliente == -1) {
        if (errno == EINTR) {
            continue;          /* una señal interrumpió accept: no es un error */
        }
        perror("accept");      /* un error real sí se informa */
        continue;
    }
    ...
}
```

`EINTR` no se trata como error: solo significa "llegó una señal, vuelve a
mirar las banderas". Si se abandonara el ciclo con un mensaje, un simple
`SIGALRM` (el informe periódico) detendría el servidor.

Al salir del ciclo, el servidor cierra el socket de escucha (con
`close`, comprobando su retorno) y presenta los contadores. Prueba con la
carga del docente (ocho lectores con 20 lecturas y dos escritores con 10
escrituras, política `rwlock`):

```
Senal de terminacion recibida: se cierra el servidor
Lecturas atendidas: 160
Escrituras atendidas: 20
```

160 = 8 × 20 y 20 = 2 × 10: los contadores coinciden exactamente con lo
que enviaron los clientes. El código de salida es 0 y el puerto 5500
queda libre.

También se probó con clientes conectados y trabajando en el momento del
`SIGINT` (política `mutex`): el servidor termina, presenta sus contadores
(`Lecturas atendidas: 3`, `Escrituras atendidas: 3` en esa corrida) y los
clientes informan "La conexión se interrumpió en la repetición 4". Es el
mismo efecto sobre los clientes que en el punto 1, pero esta vez el
servidor sí cerró su socket y dio cuentas de lo que hizo.

### Los hilos de los clientes no reciben las señales

Una señal dirigida al proceso (como la de `kill` o la de Ctrl-C) puede
entregarse a **cualquier hilo** que no la tenga bloqueada. POSIX no
garantiza que sea el principal. Si la recibiera un hilo de cliente
bloqueado en `read`, esa llamada retornaría `EINTR` (se interpretaría como
cliente desconectado) y `accept` del hilo principal no se interrumpiría.
Para no depender de que el núcleo elija al hilo principal, `main` bloquea
`SIGINT` y `SIGALRM` justo antes de `pthread_create` y restaura la
máscara justo después (`crear_hilo_cliente()`): el hilo nuevo hereda la
máscara de su creador, así que nace con ambas señales bloqueadas y solo el
hilo principal las recibe.

### Limitación conocida

Entre la comprobación `while (!terminar)` y la entrada a `accept` hay una
ventana de instrucciones en la que, si justo llega la señal, la bandera se
activa pero `accept` se bloquea de todos modos. Para `SIGINT` el retraso
está acotado: la siguiente alarma (como máximo 5 s) interrumpe `accept` y
el ciclo ve la bandera. Para `SIGALRM`, un informe perdido de esa forma
quedaría pendiente hasta que otra señal o una conexión despierte a
`accept`. La solución completa es esperar con `pselect`/`ppoll`, que
desbloquean la señal y esperan de forma atómica, pero el enunciado pide
explícitamente el esquema con `accept` y `EINTR`, y la ventana es de unas
pocas instrucciones cada 5 segundos, así que se dejó como está.

## Punto 4 — protección de los contadores

**Mecanismo elegido:** variables atómicas de C11,

```c
static atomic_long lecturas_atendidas;
static atomic_long escrituras_atendidas;
...
atomic_fetch_add(&lecturas_atendidas, 1);
```

**Criterio.** Lo que cada hilo hace con un contador es sumarle uno, y lo
único que se hace con su valor es leerlo al final. Esa operación es
"leer, sumar, escribir": si dos hilos la hacen a la vez sobre un entero
corriente, ambos leen el mismo valor y ambos escriben el mismo resultado,
así que se pierde un incremento. Una operación atómica ejecuta esos tres
pasos como uno indivisible, y es suficiente porque:

1. cada contador es independiente: no existe una relación que deba
   mantenerse entre ellos ni con el tablero (a diferencia del tablero, cuya
   regla "todas las casillas iguales" abarca 64 posiciones y por eso sí
   necesita un candado);
2. no hay ninguna sección crítica de varias instrucciones que proteger,
   solo una suma.

**Por qué no usar los candados del tablero.** Reutilizar `candado` o
`candado_rw` para los contadores haría que todos los clientes pasen por el
mismo punto de espera en cada orden, y con la política `rwlock` volvería
a serializar a los lectores, que es justo lo que esa política evita.
Un `mutex` propio de los contadores sería correcto, pero más lento que
una operación atómica, sin ventaja alguna en este caso.

**Medición.** Para comprobar el criterio se escribió un programa aparte
(no forma parte de la entrega) con 8 hilos que suman 1 000 000 de veces
cada uno sobre un contador, con tres mecanismos. Resultado en tres
corridas (se esperaban 8 000 000):

| Mecanismo | Valor final | Tiempo |
| --- | --- | --- |
| Sin protección (`volatile long`) | 1 412 950 / 1 761 147 / 2 339 993 | 0.07 s |
| `mutex` | 8 000 000 (las tres) | 0.35–0.36 s |
| Atómico | 8 000 000 (las tres) | 0.08–0.09 s |

Sin protección se pierde entre el 70 % y el 82 % de los incrementos, y
cambia entre corridas, igual que las lecturas incoherentes de la práctica
anterior. El mutex y el atómico son correctos, y el atómico cuesta una
cuarta parte.

En el servidor real los incrementos son muchísimo más espaciados (cada
lectura tarda decenas de milisegundos), así que allí la pérdida sin
protección sería rara y difícil de observar, que es lo peligroso: el
defecto sobrevive a las pruebas. Con la protección, la carga del docente
da conteos exactos con las tres políticas:

```
politica sin:    Lecturas atendidas: 160 | Escrituras atendidas: 20
politica mutex:  Lecturas atendidas: 160 | Escrituras atendidas: 20
politica rwlock: Lecturas atendidas: 160 | Escrituras atendidas: 20
```

Los contadores se incrementan **después** de que la operación sobre el
tablero termina, de modo que cuentan operaciones completadas.

## Punto 5 — `SIGALRM` y el informe periódico

```c
static volatile sig_atomic_t informar = 0;
static void al_sonar_alarma(int senal) { (void) senal; informar = 1; }
```

Se instala igual que la de `SIGINT` y se programa con
`alarm(INTERVALO_INFORME)` (5 segundos) justo antes del ciclo. El informe
lo presenta el ciclo principal, no el manejador, al observar la bandera;
vuelve a programar la alarma cada vez, porque `alarm` dispara una sola
vez:

```c
if (informar) {
    informar = 0;
    presentar_estado();      /* leer_tablero() con la política vigente */
    alarm(INTERVALO_INFORME);
}
```

`presentar_estado()` llama a `leer_tablero()`, la misma función de la
orden `LEER`, de modo que toma el candado en modo de lectura según la
política (con `rwlock`, `pthread_rwlock_rdlock`). Por eso es importante que
corra en el ciclo principal: un manejador que tomara ese candado se
bloquearía para siempre si el hilo interrumpido ya lo tuviera.

**Salida de un informe periódico** (política `rwlock`, con la carga de
ocho lectores y dos escritores en curso; un informe cada 5 segundos):

```
Tablero de 64 casillas, politica rwlock, escuchando en el puerto 5500
[informe] version 9, tablero: AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
[informe] version 18, tablero: BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB
[informe] version 20, tablero: BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB

Senal de terminacion recibida: se cierra el servidor
Lecturas atendidas: 160
Escrituras atendidas: 20
```

Se ven tres cosas: la versión avanza (9, 18, 20) a medida que los dos
escritores completan escrituras; en cada informe el tablero está lleno de
un mismo carácter, o sea que con `rwlock` la lectura del informe nunca ve
un estado intermedio; y los informes se repiten solos, lo que confirma
que la alarma se reprograma. Con un servidor sin clientes el informe
muestra `version 0` y el tablero de puntos.

Un `kill -ALRM` enviado a mano provoca un informe de inmediato, y como
`alarm` reemplaza la alarma pendiente, el reloj de 5 segundos vuelve a
contar desde ahí.

## Punto 6 — enviar las señales con `kill`

Con el servidor en segundo plano (`./servidor_tablero rwlock &`):

```
$ kill -ALRM $!     # dos veces
[informe] version 0, tablero: ................................................................
[informe] version 0, tablero: ................................................................
   (el servidor sigue vivo)

$ kill -INT $!
Senal de terminacion recibida: se cierra el servidor
Lecturas atendidas: 0
Escrituras atendidas: 0
   (codigo de salida 0)

$ kill -KILL $!
Killed                      (codigo de salida 137 = 128 + 9)
   (sin mensaje de cierre ni contadores)
```

| Señal | Efecto |
| --- | --- |
| `SIGALRM` | informe inmediato del tablero; el servidor sigue |
| `SIGINT` | terminación ordenada: cierra el socket y presenta contadores; código 0 |
| `SIGKILL` | el proceso desaparece sin ejecutar una sola línea más: sin mensaje, sin contadores; el shell informa `Killed` y código 137 |

**Intento de capturar `SIGKILL`.** Un programa de prueba (aparte) intentó
instalar un manejador para `SIGINT`, `SIGKILL` y `SIGSTOP` con `sigaction`:

```
sigaction(SIGINT): instalado
sigaction(SIGKILL): Invalid argument
sigaction(SIGSTOP): Invalid argument
```

`sigaction` devuelve `-1` con `errno = EINVAL`: el sistema no permite
capturar, bloquear ni ignorar `SIGKILL` (ni `SIGSTOP`).

**Por qué no lo permite y en qué beneficia.** Si un programa pudiera
capturar `SIGKILL`, un proceso con un error (un manejador que se cuelga,
un ciclo que ignora todas las señales) o malicioso podría hacerse
imposible de terminar, y el administrador o el sistema no tendrían una
forma garantizada de quitarlo. `SIGKILL` es la salida de emergencia: el
núcleo termina el proceso él mismo, sin ejecutar código del proceso, de
modo que nada de lo que el proceso haga puede impedirlo. Eso la hace
indispensable para el administrador (`kill -9`), para el apagado del
sistema y para el mecanismo de memoria agotada (OOM killer). `SIGSTOP`
se protege por la misma razón: el sistema siempre debe poder detener un
proceso. El costo se ve en la prueba: el servidor no tiene oportunidad de
cerrar el socket ni de presentar sus contadores, por eso se debe intentar
primero con `SIGINT` o `SIGTERM`, que sí se pueden capturar, y dejar
`SIGKILL` como último recurso.

## Punto 7 — valores de retorno y `perror`

Las llamadas al sistema que el programa agrega, y qué se hace con su
retorno:

| Llamada | Cómo se informa un fallo |
| --- | --- |
| `sigaction` (dos veces, en `instalar_manejador`) | `perror("sigaction")` y el servidor no arranca |
| `close(socket_escucha)` al terminar | `perror("close")`; el código de salida pasa a `EXIT_FAILURE` |
| `signal(SIGPIPE, SIG_IGN)` (ya existía) | se compara con `SIG_ERR` y `perror("signal")` |
| `accept` (ya existía) | `perror("accept")`, salvo `EINTR`, que no es un error |
| `setsockopt`, `malloc` | se comprobaron también: `perror` |
| `pthread_create`, `pthread_detach`, `pthread_sigmask` | `strerror` (ver abajo) |
| `alarm` | no puede fallar: devuelve los segundos que quedaban de la alarma anterior |

**Por qué las funciones de pthread no usan `perror`.** Las funciones
`pthread_*` no dejan su error en `errno`: lo devuelven como valor de
retorno. El código original hacía `perror("pthread_create")`, que imprimía
el `errno` de otra llamada anterior y por tanto un mensaje equivocado. Se
corrigió con `informar_error_pthread()`, que usa `strerror(codigo)`.

**Fallo provocado a propósito:** arrancar un segundo servidor mientras el
primero escucha en el puerto 5500.

```
$ ./servidor_tablero rwlock &      # el primero queda escuchando
$ ./servidor_tablero rwlock        # el segundo
bind: Address already in use
$ echo $?
1
```

`bind` falla con `EADDRINUSE` y `perror` informa el mensaje del sistema.
Aunque el servidor activa `SO_REUSEADDR`, esa opción solo permite reutilizar
un puerto que quedó en `TIME_WAIT` tras un cierre; no permite dos sockets
**en escucha** en el mismo puerto. Sin la comprobación, el segundo servidor
seguiría adelante con un descriptor inválido y fallaría en `listen` o
`accept` con un mensaje confuso; así termina de inmediato con la causa
real. También se probó el caso de uso incorrecto (`./servidor_tablero foo`):
imprime `Uso: ./servidor_tablero sin|mutex|rwlock` y sale con código 1.

## Punto 8 — `signal(SIGPIPE, SIG_IGN)`

**Qué hace la línea.** Le dice al proceso que ignore `SIGPIPE`. Esa señal
se genera cuando un programa escribe en un socket (o una tubería) cuyo
otro extremo ya cerró la conexión, y su acción por defecto es **terminar
el proceso**. Con `SIG_IGN` la señal no se entrega y el `write` falla de
manera normal: retorna `-1` con `errno = EPIPE`. El servidor ya contempla
ese caso:

```c
if (write(socket_cliente, respuesta, strlen(respuesta)) == -1) {
    break;       /* sale del ciclo, cierra el socket y termina solo ese hilo */
}
```

**Qué ocurriría sin ella.** Un solo cliente que cierre la conexión de
golpe, en el momento en que el servidor le está respondiendo, haría que el
servidor reciba `SIGPIPE` y **muera completo**, junto con las conexiones
de todos los demás clientes. Un cliente podría dejar sin servicio a todos
los demás, aun sin quererlo.

**Comprobación.** Se compiló una variante del servidor sin esa línea y se
probó con un cliente que envía tres órdenes de golpe y cierra sin leer
nada (`ESCRIBIR A`, `LEER`, `LEER`). El primer `write` del servidor llega
a un socket cuyo otro extremo ya cerró y recibe un `RST`; el siguiente
`write` falla con `EPIPE`. Se repitió 8 veces por variante:

```
servidor_tablero (con SIG_IGN):  sobrevivio 8 veces y murio 0 veces
servidor_sin_ign (sin SIG_IGN):  sobrevivio 3 veces y murio 5 veces (codigo 141 = 128 + 13 = SIGPIPE)
```

Con la línea, el servidor aguanta las 8 veces. Sin ella murió 5 de 8:
el resultado depende de la temporización (al parecer, de cuándo llega el
`RST` respecto de la lectura de la orden siguiente), por eso no es 8 de 8. Es el mismo tipo de
defecto de temporización que la práctica anterior mostraba con las
lecturas incoherentes: no se manifiesta siempre, y por eso conviene
cerrar el caso con una protección y no con la esperanza de que no ocurra.
Con `strace` se ve la secuencia exacta de la variante:

```
write(4, "OK 1 AAAA..."..., 70) = -1 EPIPE (Broken pipe)
--- SIGPIPE {si_signo=SIGPIPE, ...} ---
+++ killed by SIGPIPE +++
```

## Lista de comprobación de la entrega

- [x] `servidor_tablero.c` modificado, compilando sin advertencias con
  `gcc -Wall -Wextra`.
- [x] Bitácora con las respuestas de los puntos 1, 4, 6, 7 y 8 (y también
  de los puntos 2, 3 y 5), y con la salida de un informe periódico.
- [x] El manejador solo registra el hecho; el trabajo ocurre en el ciclo
  principal.
- [x] El servidor termina cerrando el socket de escucha y presentando sus
  contadores.
- [x] Contadores compartidos protegidos con un criterio justificado
  (atómicos, medidos contra el mutex).
- [x] Errores de las llamadas al sistema informados.
- [x] Documentación Doxygen generada sin advertencias (`doxygen Doxyfile`).
