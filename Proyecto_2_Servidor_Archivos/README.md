# Proyecto 2: servidor de archivos

Edward Esteban Dávila Salazar y Miguel Angel Perez Mera (@miguelperez80). Laboratorio de Sistemas Operativos, segundo corte.

Un servidor y un cliente en C que mueven archivos por la red con sockets TCP. El servidor atiende varios clientes a la vez, un hilo por cada uno. Funcionan `put`, `get`, `list` y `exit`. Al conectarse el cliente dice su usuario (no hay clave, solo identifica).

Compilar:

```bash
make
```

Correr, cada uno en su terminal:

```bash
./server              # puerto 1234, o ./server 5000
./client              # 127.0.0.1:1234, pide el usuario; o ./client 192.168.1.10 5000 ana
```

En el cliente se escribe `put ARCHIVO`, `get ARCHIVO`, `list` o `exit`. Ctrl-D es lo mismo que `exit`, y Ctrl-C o SIGTERM cierran el cliente y el servidor sin dejar archivos a medias. Cada usuario tiene su carpeta (`files/USUARIO/`, junto al `server`) y solo ve y toca lo suyo. Lo que se baja queda en `client_files/`, junto al `client`.

`split`, `util` y `netutil` son del profesor y no se tocaron. El avance día a día está en `bitacora.md`.

La documentación con Doxygen se genera con `make doc` y queda en `docs/html/index.html`.
