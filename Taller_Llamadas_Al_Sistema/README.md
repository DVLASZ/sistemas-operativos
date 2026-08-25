# Taller: llamadas al sistema

Laboratorio de Sistemas Operativos. Dos programas en C que distinguen una
llamada al sistema (`write`, `getpid`, `getppid`) de una función de
biblioteca (`printf`), verificando siempre el valor de retorno.

## Contenido

- `programa1.c` — obtiene el pid propio y el del proceso padre, escribe un
  mensaje en la salida estándar usando `write()` (no `printf()`), verifica
  el retorno de cada llamada y termina con un código de salida distinto
  de cero.
- `programa2.c` — imprime el mismo texto dos veces, una con `printf()` y
  otra con `write()`, sin `fflush` ni salto de línea, para observar el
  orden real de entrega a la terminal.
- `bitacora.md` — documentación de la práctica, con la explicación de por
  qué los mensajes aparecen en ese orden (punto 5 del enunciado).
- `Doxyfile` — configuración de Doxygen para generar la documentación de
  los fuentes a partir de los comentarios `/** ... */`.

## Compilar

```bash
gcc -Wall -Wextra -std=c11 -o programa1 programa1.c
gcc -Wall -Wextra -std=c11 -o programa2 programa2.c
```

Requiere un sistema Unix/Linux (usa `getppid()`, no disponible en
Windows/MinGW).

## Generar la documentación

```bash
doxygen Doxyfile
```

Genera `docs/html/index.html` a partir de los comentarios del código.
Esa carpeta no se versiona (ver `.gitignore`): se regenera localmente
cuando se necesita.

## Requisitos

- `gcc`
- `doxygen` (solo para generar la documentación; no es necesario para
  compilar ni ejecutar los programas)
