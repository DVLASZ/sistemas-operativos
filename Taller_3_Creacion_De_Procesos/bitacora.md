# Bitácora — Taller: creación de procesos

**Autores:** Edward Esteban Davila Salazar, Miguel Angel Perez Mera

**Entorno de verificación:** Debian 13 (trixie), `gcc 14.2.0`, máquina
virtual de VirtualBox del laboratorio de la materia.

## Documentación consultada

```
man 2 fork      -> fork(2)
man 3 exec      -> execvp(3) y el resto de la familia exec
man 2 waitpid   -> waitpid(2)
man 2 wait      -> wait(2), macros WIFEXITED/WEXITSTATUS/WIFSIGNALED/WTERMSIG
```

## El programa

`procesos.c` crea dos hijos con `fork()` + `execvp()` — uno corre
`sleep 2`, el otro `echo "Hola desde el segundo hijo"` — y el padre
espera a ambos con `waitpid(-1, &status, 0)`, informando el código de
salida de cada uno y cuál terminó primero.

```bash
make
./procesos
```

```
Proceso principal, pid=12048
[hijo1] padre (pid 12048): cree al hijo 12049
[hijo1] hijo (pid 12049): voy a ejecutar 'sleep'
[hijo2] padre (pid 12048): cree al hijo 12050
[hijo2] hijo (pid 12050): voy a ejecutar 'echo'
Hola desde el segundo hijo
[hijo2] (pid 12050) termino con codigo de salida 0 -- fue el primero en terminar
[hijo1] (pid 12049) termino con codigo de salida 0
```

Se ven las tres ramas de `fork()` en juego: la del **hijo** (imprime su
propio pid y qué va a ejecutar, antes de reemplazarse con `execvp`), la
del **padre** (imprime que creó al hijo y sigue corriendo el mismo
programa), y la de **error** está contemplada en el código
(`fork() == -1`) aunque no se pudo forzar de forma realista para esta
corrida —`fork()` prácticamente no falla en un sistema con recursos
disponibles; fallaría, por ejemplo, si se agotara el límite de
procesos del usuario (`ulimit -u`) o la memoria del sistema—.

`echo` termina casi instantáneamente; `sleep 2` tarda dos segundos
reales. Por eso `hijo2` (el `echo`) es sistemáticamente el que informa
"fue el primero en terminar", y no por casualidad ni por el orden en
que se crearon los procesos: `waitpid(-1, ...)` espera a **cualquier**
hijo, así que el que retorna primero es, genuinamente, el que terminó
primero.

## Un hallazgo antes de llegar al punto 3: salida duplicada

La primera vez que se corrió el programa (por SSH, sin terminal
interactiva asignada), la salida salió duplicada:

```
Proceso principal, pid=11959
[hijo1] hijo (pid 11960): voy a ejecutar 'sleep'
Proceso principal, pid=11959
[hijo1] padre (pid 11959): cree al hijo 11960
[hijo2] hijo (pid 11961): voy a ejecutar 'echo'
Hola desde el segundo hijo
Proceso principal, pid=11959
[hijo1] padre (pid 11959): cree al hijo 11960
[hijo2] padre (pid 11959): cree al hijo 11961
...
```

La causa conecta directamente con el taller anterior (buffer de
`stdio` vs. escritura directa): `stdout` es *line-buffered* solo cuando
está conectado a una terminal interactiva; si se redirige (a un
archivo, una tubería, o —como en esta prueba— una sesión sin terminal
asignada), pasa a ser *fully-buffered*, y el texto se acumula en un
buffer en memoria sin escribirse hasta que ese buffer se llena o se
vacía explícitamente.

`fork()` duplica la memoria del proceso **tal como está en ese
instante**, incluido lo que hubiera quedado pendiente en el buffer de
`stdout` sin escribirse. Si el padre no vació su buffer antes de cada
`fork()`, cada hijo hereda su propia copia de ese contenido pendiente,
y lo vuelve a escribir cuando su copia del buffer se vacía —de ahí que
"Proceso principal, pid=11959" apareciera repetido: no es el kernel
imprimiéndolo de más, es el mismo texto sin escribir, duplicado en la
memoria de cada hijo por el propio `fork()`.

Se corrigió forzando el modo *line-buffered* para `stdout` al inicio
de `main()`:

```c
setvbuf(stdout, NULL, _IOLBF, 0);
```

así cada `printf()` que termina en `\n` se vacía de inmediato, sin
importar si la salida está conectada a una terminal o no, y no queda
nada pendiente en el buffer al momento de cada `fork()`.

## Punto 3: código después de exec

Se agregó código justo después de la llamada a `execvp()`, dentro de
la rama del hijo:

```c
execvp(argv[0], argv);

/* Solo se llega aca si execvp() fallo... */
fprintf(stderr, "[%s] execvp de '%s' fallo: %s\n",
        etiqueta, argv[0], strerror(errno));
_exit(EXIT_FAILURE);
```

**Corriendo el programa tal como se entrega** (comandos válidos,
`sleep` y `echo`), ese `fprintf`/`_exit` nunca aparece en la salida —se
puede confirmar en la corrida de más arriba: no hay ningún mensaje de
`execvp` fallido.

**Por qué no se ejecuta:** cuando `execvp()` tiene éxito, no *retorna*
en el sentido normal de una función — reemplaza por completo la imagen
del proceso que la llamó: su código, su pila, sus datos, todo. El
proceso sigue existiendo (mismo PID, mismos descriptores de archivo
abiertos), pero deja de ejecutar `procesos.c` y pasa a ejecutar
`/usr/bin/sleep` o `/usr/bin/echo` desde su propio `main()`. El código
que estaba escrito después de `execvp()` en el código fuente de
`procesos.c` ya no forma parte de lo que ese proceso está corriendo:
no es que se "salte" por alguna condición, es que ese proceso, tal
como estaba definido en `procesos.c`, dejó de existir en el momento en
que `execvp()` tuvo éxito.

**En qué caso sí se ejecutaría:** únicamente si `execvp()` **falla**.
Ahí sí retorna (con -1), la imagen del proceso no se reemplazó, y la
ejecución continúa con la instrucción siguiente dentro del mismo
`procesos.c`. Se comprobó forzando esto a propósito, reemplazando
temporalmente el comando del primer hijo por uno inexistente:

```c
char *cmd1[] = {"comando_que_no_existe", (char *)NULL};
```

```
Proceso principal, pid=12022
[hijo1] padre (pid 12022): cree al hijo 12023
[hijo1] hijo (pid 12023): voy a ejecutar 'comando_que_no_existe'
[hijo2] padre (pid 12022): cree al hijo 12024
[hijo1] execvp de 'comando_que_no_existe' fallo: No such file or directory
[hijo2] hijo (pid 12024): voy a ejecutar 'echo'
[hijo1] (pid 12023) termino con codigo de salida 1 -- fue el primero en terminar
Hola desde el segundo hijo
[hijo2] (pid 12024) termino con codigo de salida 0
```

Ahí sí aparece `"[hijo1] execvp de 'comando_que_no_existe' fallo: No
such file or directory"`, y el proceso termina con código de salida 1
(el que se le pasó a `_exit(EXIT_FAILURE)`) — confirmando que ese
código solo corre cuando `execvp()` no reemplazó la imagen del
proceso. Después de esta prueba se revirtió el comando a `sleep 2`
para la versión entregada.
