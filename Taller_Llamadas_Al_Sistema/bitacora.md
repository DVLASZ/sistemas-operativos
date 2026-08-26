# Bitácora — Taller: llamadas al sistema

**Autores:** Edward Davila, Miguel Angel Perez Mera

**Entorno de verificación:** Debian 13 (trixie), `gcc 14.2.0`, corriendo en
una máquina virtual de VirtualBox (la del laboratorio de la materia).

## Documentación consultada

Antes de escribir el código se consultó el manual, indicando siempre la sección:

```
man 2 getpid    -> getpid(2), getppid(2): identificadores de proceso
man 2 write     -> write(2): escribir en un descriptor de archivo
man 3 printf    -> printf(3): función de biblioteca con formato
man 3 perror    -> perror(3): reporte de errores basado en errno
```

`man 2 write` deja explícito que `write()` puede escribir **menos bytes** de
los pedidos (devuelve la cantidad de bytes efectivamente escritos, no un
booleano de éxito), y que en error devuelve `-1` y fija `errno`. Ese
comportamiento es la base del punto 2 del enunciado y del error frecuente
"ignorar el valor de retorno de write".

## Programa 1 — `programa1.c`

Obtiene `pid` propio con `getpid()` y `ppid` del padre con `getppid()`,
arma el mensaje en un buffer con `snprintf` (formateo en memoria, no es
E/S) y lo entrega a la terminal exclusivamente con `write()`. Se usa una
función auxiliar `escribir_todo()` que reintenta mientras `write()` no
haya escrito el buffer completo (cubre tanto escrituras parciales como la
interrupción por señal, `EINTR`). Ante error real se llama a `perror` y se
termina con `EXIT_FAILURE`. Si todo sale bien, el proceso termina con el
código **7**, elegido arbitrariamente para distinguir una terminación
normal de este programa de un simple `exit(0)`.

Compilación sin advertencias, en Debian 13 (VM de laboratorio, kernel
`6.12.101+deb13-amd64`):

```bash
$ gcc -Wall -Wextra -std=c11 -o programa1 programa1.c
```

Ejecución y verificación del código de salida:

```bash
$ ./programa1
Proceso pid=4590, proceso padre ppid=4584
$ echo $?
7
```

El `ppid` mostrado corresponde al proceso de la shell que lanzó el
programa (visible con `ps` como el mismo `bash`/`sshd` de la sesión),
confirmando que `getppid()` efectivamente devuelve el identificador del
proceso padre real.

## Programa 2 — `programa2.c`

Imprime el mismo texto dos veces, primero con `printf()` y luego con
`write()`, sin `fflush` y sin salto de línea final, tal como pide el
enunciado.

Compilación y ejecución en Debian 13:

```bash
$ gcc -Wall -Wextra -std=c11 -o programa2 programa2.c
$ ./programa2
Hola desde el taller de llamadas al sistemaHola desde el taller de llamadas al sistema
$ echo $?
0
```

Como el texto es idéntico en ambas llamadas (así lo exige el enunciado),
no se distingue a simple vista cuál de las dos copias llegó primero a la
pantalla con solo mirar la terminal. La forma de comprobar el orden real
de entrega al kernel es con `strace`, filtrando por la llamada `write`:

```bash
$ strace -e trace=write ./programa2
write(1, "Hola desde el taller de llamadas"..., 43) = 43
write(1, "Hola desde el taller de llamadas"..., 43) = 43
+++ exited with 0 +++
```

Se ven dos llamadas `write(1, ...)` reales, cada una de 43 bytes (el
largo exacto del texto). La primera es la `write()` explícita que llama
el programa; la segunda no está escrita en ningún lado del código
fuente — la genera internamente la biblioteca de E/S estándar al vaciar
el buffer de `stdout` en el momento en que el proceso termina. `strace`
confirma así, a nivel de llamadas al sistema, la explicación del punto 5:
el `write()` del programador llega al kernel de inmediato, mientras que
el texto de `printf()` viaja "escondido" en un buffer de biblioteca hasta
el final del proceso, momento en el que la propia biblioteca ejecuta su
propio `write()` para vaciarlo.

## Punto 5 — por qué aparecen en ese orden

En el código, `printf()` se llama **antes** que `write()`. Sin embargo, a
nivel del sistema, el mensaje de `write()` llega antes que el de
`printf()`. La razón está en dónde queda el texto antes de llegar a la
pantalla:

- `printf()` es una función de **biblioteca** (sección 3 del manual). No
  entrega los bytes al kernel de inmediato: los acumula en un buffer
  propio de la biblioteca de E/S estándar (`stdio`), asociado al
  `FILE *stdout`. Ese buffer se vacía (se traduce en una llamada real a
  `write()`) recién cuando se llena, cuando se pide explícitamente con
  `fflush`, o cuando el proceso termina normalmente. Como el texto no
  tiene salto de línea y no se llamó a `fflush`, el buffer no se vacía en
  el momento de llamar a `printf()`: el mensaje queda "en espera" dentro
  del proceso.

- `write()` es una **llamada al sistema** (sección 2 del manual): cruza
  de inmediato al kernel y entrega los bytes al descriptor de archivo en
  el momento en que se invoca, sin pasar por ningún buffer intermedio de
  biblioteca.

Por eso, aunque `write()` se invoca después que `printf()` en el código
fuente, sus bytes llegan primero a la salida estándar: no tuvieron que
esperar en ningún buffer. El mensaje de `printf()` solo se hace visible
al final, cuando el proceso termina y la biblioteca vacía su buffer antes
de cerrar el `stdout`. Es la misma distinción central del taller: `printf`
no es una llamada al sistema, es una función de biblioteca que retrasa la
entrega real de los datos.

## Errores que se evitaron a propósito

- No se asumió que `printf` es una llamada al sistema: se usó `write()`
  para la E/S real en `programa1.c`, y `printf` solo donde el enunciado
  lo pide explícitamente (`programa2.c`).
- A `write()` se le pasa la cantidad de **bytes** a escribir
  (`strlen(texto)` o la longitud devuelta por `snprintf`), no una
  cantidad de caracteres asumida a mano.
- El valor de retorno de `write()` se verifica siempre, con un bucle que
  cubre el caso de escritura parcial (`write()` puede devolver menos
  bytes de los pedidos) y el caso de interrupción por señal (`EINTR`).
- Cada consulta al manual se hizo indicando la sección (`man 2 write`,
  `man 3 printf`), para no terminar en la página equivocada.
