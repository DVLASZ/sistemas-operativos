#!/bin/bash
#
# convertir.sh - convierte por lotes todos los archivos .mkv de un
# directorio a .mp4 (mismo nombre), con ffmpeg. No vuelve a convertir un
# archivo cuyo .mp4 ya exista.
#
# Taller: procesos, scripts y llamadas al sistema.
# Laboratorio de Sistemas Operativos.
#
# Autores: Edward Esteban Davila Salazar, Miguel Angel Perez Mera
#
# Uso: ./convertir.sh [directorio]
#   Sin argumento, usa el directorio actual.

# Si el patron *.mkv no coincide con nada, que se expanda a una lista
# vacia en lugar de pasarse sin expandir al ciclo for.
shopt -s nullglob

directorio=${1:-.}

if [ ! -d "$directorio" ]; then
    echo "convertir.sh: '$directorio' no existe o no es un directorio" >&2
    exit 1
fi

convertidos=0
saltados=0

for entrada in "$directorio"/*.mkv; do
    # El cambio de extension se resuelve con un operador de patrones del
    # shell (recorta la coincidencia mas corta al final), no invocando
    # sed, cut ni ningun otro programa. Funciona igual con nombres que
    # tienen varios puntos, como clase.2026.mkv.
    salida=${entrada%.mkv}.mp4

    if [ -e "$salida" ]; then
        echo "Saltando '$entrada': ya existe '$salida'"
        saltados=$((saltados + 1))
        continue
    fi

    echo "Convirtiendo '$entrada' -> '$salida'"
    if ffmpeg -loglevel error -i "$entrada" "$salida"; then
        convertidos=$((convertidos + 1))
    else
        echo "convertir.sh: fallo al convertir '$entrada'" >&2
        rm -f "$salida"
    fi
done

echo
echo "Convertidos: $convertidos"
echo "Saltados: $saltados"

exit 0
