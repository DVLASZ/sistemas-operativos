# Taller: sistemas de archivos

Autores: Edward Esteban Dávila Salazar, Miguel Angel Perez Mera

Laboratorio de Sistemas Operativos. El programa `info_archivo.c` recibe el nombre de un archivo e imprime su tamaño, tipo, permisos, contador de enlaces y fecha de última modificación. Usa `lstat`, así que sobre un enlace simbólico informa del enlace y no de su destino.

Compilar y probar:

```bash
make
./info_archivo archivo.txt
```

Las observaciones de las partes 1 y 2 y las respuestas están en `bitacora.md`.
