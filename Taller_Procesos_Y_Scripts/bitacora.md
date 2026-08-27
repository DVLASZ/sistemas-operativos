# Bitácora — Taller: procesos, scripts y llamadas al sistema

**Autores:** Edward Esteban Davila Salazar, Miguel Angel Perez Mera

**Entorno de verificación:** Debian 13 (trixie), `gcc 14.2.0`, `bash`,
máquina virtual de VirtualBox del laboratorio de la materia. Usuario de
trabajo: `DVLASZ` (login institucional, con permisos de administrador
vía el grupo `sudo`), tal como pide la preparación del taller.

## Documentación consultada

```
man 5 proc      -> formato de /proc/PID/status y del resto de /proc
man 2 open      -> open(2)
man 2 read      -> read(2)
man 2 close     -> close(2)
man 2 getpid    -> getpid(2), getppid(2)
man 1 bash      -> operadores de subcadena y de patrones (${var#patron}, ${var%patron})
man 1 stat      -> stat(1)
man 1 wc        -> wc(1)
man 1 ps        -> ps(1)
```

## Punto 6 — `stat -c %s` vs `wc -c` sobre `/proc/self/status`

Resultado obtenido:

```bash
$ stat -c %s /proc/self/status
0
$ wc -c < /proc/self/status
1495
```

Las dos órdenes preguntan cosas distintas. `stat` no lee el archivo: consulta
los **atributos** que el sistema de archivos guarda sobre él, entre ellos el
tamaño registrado en el inodo. Los archivos de `/proc` no tienen contenido
real en disco — son una interfaz: el kernel genera el texto en el momento
en que alguien los lee, a partir del estado actual del proceso. Como no
existe un contenido almacenado de antemano, tampoco existe un tamaño que
registrar, y el atributo que `stat` reporta es 0: no es una aproximación
ni un error, es la ausencia real del dato.

`wc -c`, en cambio, sí abre y lee el archivo completo, byte a byte, y
cuenta lo que efectivamente llega. Como la lectura sí dispara la
generación del contenido por parte del kernel, `wc` cuenta el tamaño real
del texto que se generó en ese instante (1495 bytes en esta corrida).

Esta es exactamente la razón por la que `infoproc.c` no puede dimensionar
su buffer a partir de `stat`: ese dato no existe para archivos de `/proc`.

## Punto 8 — cadena de procesos padre hasta el proceso 1

Recorrido obtenido con `infoproc.sh` (sin argumento, sobre sí mismo):

```
Cadena de procesos padre hasta el proceso 1 (a partir del pid 4485):
  PID 4485 -> Name: infoproc.sh   | State: R (running)  | PPid: 4484 | Threads: 1
  PID 4484 -> Name: bash          | State: S (sleeping)  | PPid: 4483 | Threads: 1
  PID 4483 -> Name: sshd-session  | State: S (sleeping)  | PPid: 4474 | Threads: 1
  PID 4474 -> Name: sshd-session  | State: S (sleeping)  | PPid: 1019 | Threads: 1
  PID 1019 -> Name: sshd          | State: S (sleeping)  | PPid: 1    | Threads: 1
  PID 1    -> Name: systemd       | State: S (sleeping)  | PPid: 0    | Threads: 1
```

Cadena: `infoproc.sh` → `bash` (la shell de la sesión) → `sshd-session`
(la sesión SSH concreta) → `sshd-session` (el proceso que atiende esa
conexión) → `sshd` (el demonio que escucha el puerto 22) → `systemd`
(proceso 1).

El **proceso 1** es `systemd`: el primer proceso que arranca el kernel al
iniciar el sistema, y el que en Debian se encarga de inicializar el resto
de servicios. Es su propio límite porque su `PPid` vale 0, y no existe
ningún `/proc/0`: el recorrido se detiene ahí porque no hay padre que
consultar, no porque el programa lo decida arbitrariamente.

## Punto 22 — por qué `infoproc.sh` e `infoproc` coinciden

Prueba sobre el mismo proceso (`sleep 300 &`, PID 7317):

```
infoproc.sh 7317 -> PID 7317 -> Name: sleep | State: S (sleeping) | PPid: 7298 | Threads: 1
infoproc    7317 -> PID 7317 -> Name: sleep | State: S (sleeping) | PPid: 7298 | Threads: 1
```

Coinciden exactamente, y tienen que coincidir: los dos programas son
clientes distintos de la **misma fuente de datos**, que es el kernel.
Ninguno de los dos mantiene su propia copia del estado de un proceso ni
lo calcula por su cuenta — ambos abren el mismo archivo virtual,
`/proc/PID/status`, y el kernel les entrega el mismo texto generado en
ese instante, sin importar si quien preguntó fue un script de shell
leyendo con `read` o un programa en C leyendo con `open`/`read`/`close`.
La diferencia entre los dos programas está únicamente en **cómo piden y
procesan** ese texto (operadores de patrones del shell vs. llamadas al
sistema y funciones de `string.h`), no en **qué** piden: el contenido de
origen es idéntico, así que el resultado final también lo es.

## Punto 23 — el mismo experimento sobre un proceso que cambia de estado

Se lanzó un proceso que alterna deliberadamente entre estar dormido
(`sleep 0.3`) y ocupar la CPU en un ciclo (~3 millones de iteraciones), y
se lo consultó varias veces seguidas, alternando `infoproc.sh` e
`infoproc` en cada ronda:

```
--- ronda 1 --- (separadas ~0.2s entre rondas)
infoproc.sh: PID 7641 -> Name: oscila.sh | State: S (sleeping) | PPid: 7638 | Threads: 1
infoproc:    PID 7641 -> Name: oscila.sh | State: S (sleeping) | PPid: 7638 | Threads: 1
--- ronda 2 ---
infoproc.sh: PID 7641 -> Name: oscila.sh | State: S (sleeping) | PPid: 7638 | Threads: 1
infoproc:    PID 7641 -> Name: oscila.sh | State: S (sleeping) | PPid: 7638 | Threads: 1
--- ronda 3 ---
infoproc.sh: PID 7641 -> Name: oscila.sh | State: R (running) | PPid: 7638 | Threads: 1
infoproc:    PID 7641 -> Name: oscila.sh | State: R (running) | PPid: 7638 | Threads: 1
--- ronda 4 ---
infoproc.sh: PID 7641 -> Name: oscila.sh | State: R (running) | PPid: 7638 | Threads: 1
infoproc:    PID 7641 -> Name: oscila.sh | State: R (running) | PPid: 7638 | Threads: 1
```

El resultado confirma **las dos partes** de la explicación del punto 22 a
la vez:

- **Dentro de cada ronda**, `infoproc.sh` e `infoproc` se ejecutan con
  una diferencia de tiempo despreciable (microsegundos), así que leen
  `/proc` prácticamente en el mismo instante y siguen coincidiendo entre
  sí, aunque el proceso consultado esté cambiando de estado.
- **Entre una ronda y otra**, separadas por about 0.2 segundos, el
  informe sí cambió: de `S (sleeping)` en las rondas 1 y 2 a
  `R (running)` en las rondas 3 y 4, porque el proceso observado
  realmente cambió de estado en el tiempo transcurrido entre una
  consulta y la siguiente.

Esto confirma que `/proc` no es una fotografía fija ni un registro
histórico: cada lectura devuelve el estado real en el instante exacto en
que el kernel atiende esa lectura. Dos programas que leen con una
diferencia de tiempo suficiente pueden legítimamente reportar cosas
distintas sobre el mismo proceso — no porque alguno se equivoque, sino
porque el proceso mismo cambió entre una lectura y la otra.

## Punto 24 — qué hace el shell entre el comando y el prompt

Cuando se escribe una línea y se presiona Enter, el shell (`bash`) hace
todo esto antes de que exista un proceso nuevo, apoyándose en servicios
del sistema operativo:

1. **Lee y analiza la línea**: la separa en palabras, identifica el
   comando, sus argumentos, y cualquier redirección (`<`, `>`, `>>`) o
   tubería (`|`). Esto es puro procesamiento de texto, todavía no
   involucra al sistema operativo.
2. **Decide de dónde sale el comando**: si es una orden interna de
   `bash` (como `cd` o `export`), la ejecuta ella misma sin crear ningún
   proceso. Si es un ejecutable externo (como `ps` o `./infoproc`), lo
   busca recorriendo los directorios de la variable `PATH`.
3. **Pide al kernel un proceso nuevo con `fork()`**: esta llamada al
   sistema crea una copia casi idéntica del propio shell (mismo código,
   mismas variables, mismos descriptores de archivo abiertos). A partir
   de ahí hay dos procesos ejecutando el mismo programa: el shell
   original (padre) y la copia (hijo).
4. **El proceso hijo configura sus flujos y reemplaza su código con
   `execve()`**: antes de ejecutar el programa pedido, si la línea traía
   redirecciones o tuberías, el hijo ajusta sus descriptores de entrada,
   salida y error (por ejemplo con `dup2`) para que apunten a los
   archivos o a la tubería correspondientes. Recién entonces llama a
   `execve()`, que reemplaza por completo el código del proceso hijo por
   el del programa pedido (`ps`, `infoproc`, etc.), conservando el mismo
   PID.
5. **El shell padre espera con `wait()`/`waitpid()`**: mientras el hijo
   corre, el shell padre queda bloqueado esperando a que termine (salvo
   que la línea terminara en `&`, caso en el que no espera y el proceso
   queda en segundo plano). El sistema operativo se encarga de
   despertarlo cuando el hijo termina.
6. **Recoge el código de salida y vuelve a mostrar el prompt**: el shell
   lee el estado de terminación que le entrega `wait()`, lo deja
   disponible en `$?`, y vuelve a leer la entrada estándar, mostrando de
   nuevo el símbolo de espera.

En resumen: el shell en sí mismo no "ejecuta" el programa pedido — pide
al sistema operativo que le cree un proceso nuevo (`fork`), que ese
proceso cargue el programa (`execve`), y luego se queda esperando a que
el sistema operativo le avise que terminó (`wait`). Todo lo que
distingue a un comando de otro (variables de entorno heredadas,
descriptores redirigidos, el propio PID) ya quedó decidido por el shell
*antes* de pedirle al kernel que arranque el programa.
