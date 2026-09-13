# Taller: planificación de procesos

**Autores:** Edward Esteban Davila Salazar, Miguel Angel Perez Mera ([@miguelperez80](https://github.com/miguelperez80))

Laboratorio de Sistemas Operativos. Resolver a mano la planificación de
un conjunto de procesos con FIFO, SJF y Round Robin, observar el
reparto real del procesador en el sistema (`top`, `ps`, `renice`), y
completar un simulador en C++ que implementa esos algoritmos más SRT y
colas de prioridad.

## Contenido

- `bitacora.md` — la Parte 1 (tablas, diagramas y comparación de
  algoritmos), la Parte 2 (`top`/`ps`/`renice`), y la explicación
  técnica de qué línea de código distingue a cada algoritmo en el
  simulador.
- `diagramas_parte1/` — los 5 diagramas de Gantt de la Parte 1 (FIFO,
  SJF, RR con quantum 2, 1 y 8), generados con el propio simulador ya
  corregido, usado como verificación de los cálculos a mano.
- `planificador_edwarddavila_miguelperez/` — el proyecto del simulador,
  provisto por el docente, con los 6 `\todo` de `src/planificador.cpp`
  resueltos (ver su propio `README.md` para el enunciado completo del
  proyecto).

## Compilar y ejecutar el simulador

```bash
cd planificador_edwarddavila_miguelperez
make
./planificador test/taller_fifo.txt
./planificador -t test/taller_srt.txt   # con traza de decisiones
```

Requiere `gnuplot` para generar los diagramas de Gantt (`sudo apt
install gnuplot`); sin él, el simulador igual produce los resultados
numéricos.

## Requisitos

- `g++` (C++17), `make`
- `gnuplot` (para los diagramas)
- `doxygen` (opcional, para generar la documentación del proyecto con `make doc`)
