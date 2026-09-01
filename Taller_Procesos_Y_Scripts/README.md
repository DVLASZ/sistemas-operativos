# Taller: procesos, scripts y llamadas al sistema

**Autores:** Edward Esteban Davila Salazar, Miguel Angel Perez Mera ([@miguelperez80](https://github.com/miguelperez80))

Laboratorio de Sistemas Operativos. Observar los procesos vivos del
sistema desde el shell y desde un programa propio en C, comparando lo
que informa `/proc` por las dos vías, y resolver con un script una
tarea de administración por lotes.

## Contenido

- `infoproc.sh` — script de shell: informa Name, State, PPid y Threads
  de un proceso (o de sí mismo, si no recibe pid) leyendo
  `/proc/PID/status`, y recorre la cadena de procesos padre hasta el
  proceso 1. Extrae los valores con operadores de patrones del shell,
  sin invocar otros programas.
- `infoproc.c` — el mismo informe que `infoproc.sh` para un único
  proceso, pero leyendo `/proc/PID/status` exclusivamente con
  `open`/`read`/`close` (sin `fopen`/`fgets`), verificando el retorno
  de cada llamada.
- `convertir.sh` — convierte por lotes todos los `.mkv` de un
  directorio a `.mp4` con `ffmpeg`, sin reconvertir los que ya existen.
- `Makefile` — publicado con el enunciado; construye `infoproc`
  (`make`) y limpia lo generado (`make clean`).
- `Doxyfile` — configuración de Doxygen para generar la documentación
  de `infoproc.c` a partir de sus comentarios `/** ... */`.
- `bitacora.md` — documentación de la práctica, con capturas de cada
  avance. La mayor parte se tomó en la VM Debian de Edward; la parte 5
  incluye además evidencia de Miguel, resuelta en su propia VM con
  Ubuntu.
- `capturas/` — capturas de pantalla referenciadas desde `bitacora.md`.

## Compilar y ejecutar

```bash
make
./infoproc              # informa sobre si mismo
./infoproc 1234          # informa sobre el proceso 1234
make clean
```

```bash
chmod +x infoproc.sh convertir.sh
./infoproc.sh             # informa sobre si mismo y su cadena de padres
./infoproc.sh 1234
./convertir.sh [directorio]   # por defecto, el directorio actual
```

## Generar la documentación de infoproc.c

```bash
doxygen Doxyfile
```

Genera `docs/html/index.html`. No se versiona (ver `.gitignore` de la
raíz del repo): se regenera localmente cuando se necesita.

## Requisitos

- `gcc`, `make` (para `infoproc.c`)
- `bash` (para los scripts)
- `ffmpeg` (solo para `convertir.sh`)
- `doxygen` (solo para generar la documentación de `infoproc.c`)
