# Taller: señales en un servidor concurrente

**Autores:** Edward Esteban Dávila Salazar, Miguel Angel Perez Mera ([@miguelperez80](https://github.com/miguelperez80))

Laboratorio de Sistemas Operativos. Se extiende el servidor TCP del
tablero compartido (lectores y escritores sobre sockets) para que capture
dos señales y para que informe los errores de las llamadas al sistema:

- `SIGINT` (Ctrl-C): el servidor termina de forma ordenada, cierra el
  socket de escucha y presenta cuántas lecturas y escrituras atendió.
- `SIGALRM`: cada 5 segundos el servidor presenta la versión del tablero
  y su contenido, y vuelve a programar la alarma.
- Los contadores de lecturas y escrituras, que comparten todos los hilos
  de los clientes, quedan protegidos con operaciones atómicas.
- Las llamadas al sistema que el programa agrega se comprueban y sus
  errores se informan con `perror`.

## Contenido

- `src/servidor_tablero.c` — el servidor, con el trabajo del taller
  (único archivo del código que se modificó).
- `src/cliente_tablero.c`, `src/carga.sh`, `src/Makefile` — cliente de
  carga, guion de carga y Makefile de la práctica anterior, sin cambios.
- `Doxyfile` — configuración de Doxygen para generar la documentación de
  `src/`.
- `bitacora.md` — respuestas de los puntos 1 a 8, con la salida de un
  informe periódico y las pruebas que respaldan cada afirmación.

## Compilar y ejecutar

```bash
cd src
make                        # gcc -Wall -Wextra, sin advertencias
./servidor_tablero rwlock   # o sin | mutex
```

En otra terminal:

```bash
chmod +x carga.sh
./carga.sh 127.0.0.1 8 20   # ocho lectores y dos escritores simultáneos
kill -ALRM <pid>            # informe inmediato del tablero
kill -INT <pid>             # terminación ordenada (equivale a Ctrl-C)
```

Al terminar el servidor imprime las lecturas y escrituras atendidas.

## Generar la documentación

```bash
doxygen Doxyfile
```

Genera `docs/html/index.html`. No se versiona (ver `.gitignore` de la
raíz del repositorio): se regenera cuando se necesita.

## Requisitos

- `gcc` con soporte de C11 (`<stdatomic.h>`), `make`
- `doxygen` (solo para generar la documentación)
