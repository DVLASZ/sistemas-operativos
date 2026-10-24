# Bitácora del proyecto 2: servidor de archivos

Edward Esteban Dávila Salazar y Miguel Angel Perez Mera

## 20 de octubre

Leímos el PDF y el zip del profe. El zip trae esqueletos: el servidor atiende a un solo cliente y con el puerto fijo en 1234, el cliente apenas se conecta, y `send_file` y `receive_file` son puros comentarios. Lo que sí viene hecho es `split`, `util` y `netutil`.

Lo que hay que lograr, resumido: un servidor y un cliente, carpeta `files/` en el servidor y `client_files/` en el cliente (las dos se crean si no existen), comandos `put`, `get` y `exit`, un hilo por conexión con el arreglo de clientes protegido por un mutex, y qué hacer si dos clientes mandan el mismo nombre a la vez. También terminar bien con SIGINT y SIGTERM, y entregar un zip con un README.txt con nombres, códigos y correos.

Para los nombres repetidos nos quedamos con el sufijo (`_1`, `_2`) y no con rechazar. Un `put` suelto de un nombre que ya existe sobrescribe, como dice el PDF.

Mucho se parece al chat del taller 6 (hilo por cliente, lista con mutex) y a lo de señales del taller 5, así que partimos de ahí. Lo nuevo de verdad es el protocolo de `put` y `get`.

Dividimos el trabajo en cinco commits: la base, `put`, `get`, señales y concurrencia, y la entrega.

## 24 de octubre

Primer commit, la base. El servidor acepta varios clientes, cada uno en su hilo, los mete en un arreglo de 256 posiciones con mutex y los saca al terminar. Muestra la IP, el puerto y cuántos hay conectados. El puerto sale de la línea de comandos. El cliente lee los comandos con el `split` del profe, y todo viaja como una `request` y una `response` de tamaño fijo. Para eso hice `read_all` y `write_all`, que repiten `read` y `write` hasta mover los bytes exactos, porque un `read` en un socket puede devolver menos de lo pedido.

Lo que falló o hubo que corregir:

- Al compilar con `-Wall -Wextra` en la VM salieron tres advertencias: dos de `snprintf` por posible truncado (en `server.c` y `client.c`) y una de un parámetro sin usar en `netutil.c`. Arreglé las mías revisando lo que devuelve `snprintf`.
- El servidor no mostraba nada cuando redirigía la salida a un archivo, porque la salida quedaba en el búfer hasta cerrar. Puse `setvbuf` por línea, igual que me pasó en el taller 3.
- Había modificado `util.c`, `util.h` y `netutil.c` del profe para agregar mis funciones. No quedó bien, así que los dejé idénticos al zip y pasé `ensure_dir` y `program_dir` a un archivo mío, `fsutil.c`. En el `Makefile` esas librerías del profe se compilan con `-w` para que sus advertencias de siempre no tapen las nuestras.
- Pasé una revisión del código con una herramienta y salieron cosas útiles. Arreglé las que eran baratas: el `printf` dentro de la sección crítica (ahora copio los datos, suelto el mutex y recién imprimo), un `split` que puede devolver nulo, la dirección y el socket que no se liberaban si el cliente fallaba al conectar, y una ruta cortada en `program_dir`.
- `split` del profe no controla cuántas palabras guarda (255 como máximo). Una línea con más palabras se sale del arreglo. No quise tocar su archivo, así que el cliente cuenta las palabras antes y rechaza la línea si son demasiadas.

Pruebas en la VM:

- El `make` ya no da advertencias.
- `./server abc`, `./server 70000` y `./server 1 2 3` terminan con código 1 y un mensaje.
- Cuatro clientes a la vez se registran y se quitan bien, y el contador termina en 0.
- Una línea de 300 palabras en el cliente se rechaza sin romper nada.
- Con el servidor lanzado desde `/`, `files/` apareció junto al ejecutable y no en `/`.
- Abrí 300 conexiones al mismo tiempo. El servidor registró 256 y rechazó 44, y al final lo conectado menos lo desconectado dio 0.

Todavía no probé nada con archivos reales, porque `put` y `get` no están.

## Lo que sigue

- Commit 2: `put` completo. Falta decidir cómo validar el nombre (rechazar `/` y `..`) y pasar el tamaño de `int` a 64 bits.
- Commit 3: `get`, con el tamaño -1 cuando no existe, y las líneas largas en el cliente.
- Commit 4: SIGINT y SIGTERM en los dos, apagado ordenado, el sufijo para nombres repetidos y un límite de espera al leer.
- Commit 5: Doxygen, comentarios, el README.txt, pruebas con varios clientes y archivos grandes, y el zip con `make clean`.

Pendiente de preguntarle al profe si el README de entrega puede ser `.md` o tiene que ser `.txt`.
