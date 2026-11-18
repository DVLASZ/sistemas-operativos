# Sistemas Operativos

Trabajos del laboratorio de Sistemas Operativos. Cada carpeta es un trabajo aparte, con su propio `README.md` donde se explica de qué trata y cómo compilarlo, y con su `bitacora.md`.

## Talleres

- [`Taller_1_Llamadas_Al_Sistema/`](./Taller_1_Llamadas_Al_Sistema): llamadas al sistema. Distinguir `write()`, `getpid()` y `getppid()` (llamadas al sistema) de `printf()` (función de biblioteca).
- [`Taller_2_Procesos_Y_Scripts/`](./Taller_2_Procesos_Y_Scripts): procesos, scripts y llamadas al sistema. Observar `/proc` desde el shell y desde C, y un script que convierte videos por lotes.
- [`Taller_3_Creacion_De_Procesos/`](./Taller_3_Creacion_De_Procesos): creación de procesos con `fork`, `exec` y `waitpid` en C.
- [`Taller_4_Planificacion_De_Procesos/`](./Taller_4_Planificacion_De_Procesos): planificación de procesos. FIFO, SJF y RR a mano, `top`, `ps` y `renice` en el sistema real, y un simulador en C++ con FIFO, SJF, RR, SRT y colas de prioridad.
- [`Taller_5_Senales_En_Servidor_Concurrente/`](./Taller_5_Senales_En_Servidor_Concurrente): señales. `SIGINT`, `SIGALRM` y `sigaction` en un servidor TCP concurrente, contadores compartidos con operaciones atómicas y errores con `perror`.
- [`Taller_6_Comunicacion_Chat/`](./Taller_6_Comunicacion_Chat): chat con sockets TCP e hilos POSIX, con un mutex para la lista de clientes, `select()` en el cliente y apodos.
- [`Taller_7_Sistemas_De_Archivos/`](./Taller_7_Sistemas_De_Archivos): sistemas de archivos. Observar discos e inodos con `df`, `stat` y `ls -li`, comparar enlaces duros y simbólicos, y un programa en C que muestra los metadatos de un archivo con `lstat`.

## Proyectos

- [`Proyecto_1_Planificador/`](./Proyecto_1_Planificador): simulador de planificación de procesos con colas de prioridad (FIFO, SJF, RR y SRT). Es el del Taller 4 con más funciones, y trae un caso propio de tres colas que muestra por qué repartir los procesos en colas cambia el resultado frente a una sola.
- [`Proyecto_2_Servidor_Archivos/`](./Proyecto_2_Servidor_Archivos): servidor y cliente en C que mueven archivos por sockets TCP, con un hilo por cliente. Tiene `put`, `get`, `list` y `exit`, usuario con clave y una carpeta de archivos por usuario.

## Entorno

El código está pensado para compilarse en Linux, en el Debian que se usa en el laboratorio. Cada README dice qué hace falta para compilar su carpeta.
