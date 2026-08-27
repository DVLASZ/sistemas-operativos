#!/bin/bash
#
# infoproc.sh - informa Name, State, PPid y Threads de un proceso vivo
# leyendo /proc/PID/status, y recorre la cadena de procesos padre hasta
# llegar al proceso 1.
#
# Taller: procesos, scripts y llamadas al sistema.
# Laboratorio de Sistemas Operativos.
#
# Autores: Edward Esteban Davila Salazar, Miguel Angel Perez Mera
#
# Uso: ./infoproc.sh [pid]
#   Sin argumento, informa sobre el propio proceso que ejecuta el script ($$).
#
# Documentacion consultada:
#   man 5 proc
#   man 1 bash (operadores de subcadena/patrones: ${var#patron}, ${var%patron})

pid=${1:-$$}

if [ ! -d "/proc/$pid" ]; then
    echo "infoproc.sh: no existe ningun proceso vivo con pid $pid" >&2
    exit 1
fi

# Lee /proc/$1/status y deja Name, State, PPid y Threads en las variables
# globales nombre/estado/ppid/hilos. Cada valor se extrae con operadores
# de patrones del shell (# y %) sobre la linea ya leida con el builtin
# "read"; no se invoca grep, awk, sed ni ningun otro programa externo.
leer_status() {
    local archivo="/proc/$1/status"
    local linea valor
    nombre=""
    estado=""
    ppid=""
    hilos=""

    while IFS= read -r linea; do
        case "$linea" in
            Name:*)
                valor=${linea#Name:}
                nombre=${valor#"${valor%%[![:space:]]*}"}
                ;;
            State:*)
                valor=${linea#State:}
                estado=${valor#"${valor%%[![:space:]]*}"}
                ;;
            PPid:*)
                valor=${linea#PPid:}
                ppid=${valor#"${valor%%[![:space:]]*}"}
                ;;
            Threads:*)
                valor=${linea#Threads:}
                hilos=${valor#"${valor%%[![:space:]]*}"}
                ;;
        esac
    done < "$archivo"
}

echo "Cadena de procesos padre hasta el proceso 1 (a partir del pid $pid):"

actual="$pid"
while :; do
    if [ ! -d "/proc/$actual" ]; then
        echo "infoproc.sh: el proceso $actual ya no existe, se corta el recorrido" >&2
        exit 1
    fi

    leer_status "$actual"
    echo "  PID $actual -> Name: $nombre | State: $estado | PPid: $ppid | Threads: $hilos"

    if [ "$actual" = "1" ]; then
        break
    fi

    actual="$ppid"
done

exit 0
