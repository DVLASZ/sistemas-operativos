# Proyecto de primer corte: planificación de procesos

**Autores:** Edward Esteban Davila Salazar, Miguel Angel Perez Mera ([@miguelperez80](https://github.com/miguelperez80))

Laboratorio de Sistemas Operativos. Simulador de planificación de
procesos con colas de prioridad, que implementa FIFO, SJF, RR y SRT, y
genera diagramas de Gantt de los resultados.

## Contenido

- `planificador_edwarddavila_miguelperez/` — el proyecto provisto por
  el docente, con los 4 `\todo` de `src/planificador.cpp` resueltos
  (ver su propio `README.md` para el enunciado completo del código
  base). Incluye `test/caso_propio.txt` y
  `test/caso_propio_una_cola.txt`, el caso propio del punto 5.
- `bitacora.md` — comprobación de resultados contra la tabla conocida,
  explicación de qué línea de código distingue a cada algoritmo, y el
  desarrollo completo del punto 5 (caso propio de 3 colas, con la
  comparación contra resolver los mismos procesos en una sola cola).

## Compilar y ejecutar

```bash
cd planificador_edwarddavila_miguelperez
make
./planificador test/taller_fifo.txt
./planificador -t test/caso_propio.txt   # con traza de decisiones
```

Requiere `gnuplot` para los diagramas de Gantt (`sudo apt install
gnuplot`); sin él, el simulador igual produce los resultados numéricos.

## Requisitos

- `g++` (C++17), `make`
- `gnuplot` (para los diagramas)
- `doxygen` (opcional, para generar la documentación con `make doc`)
