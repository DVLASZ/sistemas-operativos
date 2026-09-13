# Sistemas Operativos

Repositorio de trabajos sueltos de la materia Sistemas Operativos
(laboratorio y teoría). Cada carpeta es un trabajo independiente —
taller, práctica o ejercicio— con su propio `README.md` explicando de
qué se trata y cómo compilarlo/ejecutarlo.

## Trabajos

- [`Taller_1_Llamadas_Al_Sistema/`](./Taller_1_Llamadas_Al_Sistema) —
  taller de llamadas al sistema: distinguir
  `write()`/`getpid()`/`getppid()` (llamadas al sistema) de `printf()`
  (función de biblioteca).
- [`Taller_2_Procesos_Y_Scripts/`](./Taller_2_Procesos_Y_Scripts) —
  taller de procesos, scripts y llamadas al sistema: observar `/proc`
  desde el shell y desde C, y un script de conversión de video por
  lotes.
- [`Taller_3_Creacion_De_Procesos/`](./Taller_3_Creacion_De_Procesos) —
  taller de creación de procesos: `fork`, `exec` y `waitpid` en C.
- [`Taller_4_Planificacion_De_Procesos/`](./Taller_4_Planificacion_De_Procesos)
  — taller de planificación de procesos: FIFO/SJF/RR a mano,
  `top`/`ps`/`renice` en el sistema real, y un simulador en C++ con
  FIFO/SJF/RR/SRT y colas de prioridad.

## Entorno

El código de este repositorio está pensado para compilarse en Linux
(Debian, usado en el laboratorio de la materia). Cada carpeta indica en
su propio README los requisitos puntuales de compilación.
