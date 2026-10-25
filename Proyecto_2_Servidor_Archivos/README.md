# Proyecto 2: servidor de archivos

Edward Esteban Dávila Salazar y Miguel Angel Perez Mera (@miguelperez80). Laboratorio de Sistemas Operativos, segundo corte.

Un servidor y un cliente en C que mueven archivos por la red con sockets TCP. El servidor atiende varios clientes a la vez, un hilo por cada uno. Funcionan `put`, `get` y `exit`.

Compilar:

```bash
make
```

Correr, cada uno en su terminal:

```bash
./server              # puerto 1234, o ./server 5000
./client              # 127.0.0.1:1234, o ./client 192.168.1.10 5000
```

En el cliente se escribe `put ARCHIVO`, `get ARCHIVO` o `exit`. Ctrl-D es lo mismo que `exit`. Lo que se sube queda en `files/`, junto al `server`, y lo que se baja queda en `client_files/`, junto al `client`.

`split`, `util` y `netutil` son del profesor y no se tocaron. El avance día a día está en `bitacora.md`.
