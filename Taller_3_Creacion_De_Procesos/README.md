# Taller: creación de procesos

**Autores:** Edward Esteban Davila Salazar, Miguel Angel Perez Mera ([@miguelperez80](https://github.com/miguelperez80))

Laboratorio de Sistemas Operativos. Un programa en C que crea procesos
con `fork`, reemplaza la imagen de los hijos con `exec`, y espera su
terminación con `waitpid`.

## Contenido

- `procesos.c` — crea dos procesos hijos que ejecutan comandos
  distintos (`sleep 2` y `echo`), distinguiendo las tres ramas de
  `fork()` (error, hijo, padre), espera a ambos con `waitpid()`, e
  informa el código de salida de cada uno y cuál terminó primero.
- `Makefile` — construye `procesos` (`make`) y limpia lo generado
  (`make clean`). No fue publicado con el enunciado esta vez; se
  escribió siguiendo el mismo patrón de los talleres anteriores.
- `Doxyfile` — configuración de Doxygen para generar la documentación
  de `procesos.c` a partir de sus comentarios `/** ... */`.
- `bitacora.md` — documentación de la práctica, con la explicación del
  punto 3 (por qué el código después de un `exec` exitoso no se
  ejecuta).

## Compilar y ejecutar

```bash
make
./procesos
make clean
```

## Generar la documentación de procesos.c

```bash
doxygen Doxyfile
```

Genera `docs/html/index.html`. No se versiona (ver `.gitignore` de la
raíz del repo): se regenera localmente cuando se necesita.

## Requisitos

- `gcc`, `make`
- `doxygen` (solo para generar la documentación de `procesos.c`)
