# Taller: sistemas de archivos.
#
# make          compila info_archivo
# make clean    elimina lo generado
# make doc      genera la documentacion con Doxygen en docs/html

CC = gcc
CFLAGS = -Wall -Wextra

info_archivo: info_archivo.c
	$(CC) $(CFLAGS) -o $@ $<

doc:
	doxygen Doxyfile

clean:
	rm -f info_archivo

.PHONY: clean doc
