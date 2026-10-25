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

## 24 de octubre, más tarde

Segundo commit, `put`. Cómo quedó el intercambio: el cliente manda la `request` con el nombre, el servidor revisa el nombre y contesta si autoriza, el cliente manda la información del archivo (nombre, tamaño y modo) y después el contenido, y al final el servidor confirma. Si algo falla a mitad, el servidor avisa y cierra la conexión, porque ya no sé en qué punto quedó el flujo.

Lo hice en `send_file` y `receive_file` de `protocol.c`. Copian en bloques de 64 KB, siempre con `read_all` y `write_all`, y cuentan los bytes hasta llegar exactamente al tamaño anunciado. Si el cliente se va antes de terminar, el servidor borra el archivo que quedó a medias.

Cosas que decidí sobre la marcha:

- Pasé `size` de `int` a `int64_t` en `file_info`. Con `int` no cabían archivos de más de 2 GB. El struct mide 272 bytes por el relleno, así que lo lleno de ceros antes de mandarlo para no enviar basura de la pila.
- El nombre que llega del cliente no se cree. `valid_filename` rechaza vacío, `.`, `..` y cualquier nombre con `/`. Se revisa dos veces, al recibir la solicitud y otra vez dentro de `receive_file`, porque el segundo nombre viene en otro mensaje.
- El archivo se abre con `O_NOFOLLOW`, así que un enlace simbólico dentro de `files/` no sirve para escribir fuera.
- El cliente comprueba que el archivo existe y es regular antes de hablar con el servidor, y manda solo el nombre sin la ruta.

Pruebas en la VM, todo a la primera:

- `make` sin advertencias.
- Subí un texto, un archivo vacío, uno de 3 MB al azar, uno de 70000 ceros y uno de 50 MB. Los cinco quedaron con el mismo md5 que el original y el mismo tamaño.
- Un archivo dentro de una subcarpeta llegó sin la ruta.
- Subir otra vez `texto.txt` con otro contenido lo sobrescribió.
- Ocho clientes subiendo a la vez archivos de 10 MB: los ocho llegaron iguales.
- `put` de un archivo que no existe y de una carpeta: el cliente lo rechaza sin molestar al servidor.
- Con un programa de prueba que arma los mensajes a mano mandé los nombres `../evil`, `/etc/passwd`, `.`, `..`, `a/b` y uno vacío. Todos rechazados y no apareció nada fuera de `files/`.
- Tamaño declarado negativo, y otro caso que declara 100 bytes y manda 10 y se va: no queda ningún archivo y el servidor sigue atendiendo.
- Un enlace simbólico en `files/` apuntando a `/tmp/objetivo`: el servidor dijo que no pudo recibir y `/tmp/objetivo` quedó intacto.

No probé qué pasa si se llena el disco, solo está el camino de error en el código. Y sigue pendiente lo de dos clientes mandando el mismo nombre a la vez: hoy el último que termine gana. Eso va en el commit 4 con el sufijo.

Sigue `get`.

## 25 de octubre, tercer commit

Hice `get`. El cliente manda la solicitud con el nombre, y el servidor contesta directamente con la información del archivo y el contenido, igual que en `put` pero al revés. Si el archivo no existe manda la información con tamaño -1 y nada más, como dice el PDF. Para eso `receive_file` ahora devuelve 1 si recibió, 0 si falló y -1 si el otro lado avisó que no existe, y agregué `send_not_found`. Así el cliente usa la misma `receive_file` que el servidor para el `put`.

Casi se me pasa algo: al cambiar lo que devuelve `receive_file`, el `if (!receive_file(...))` del servidor ya no servía, porque -1 cuenta como verdadero. Lo cambié a `!= 1` antes de probar.

En el servidor el archivo se revisa con `lstat` y no con `stat`, para que un enlace simbólico dentro de `files/` no se pueda usar para bajar cosas de otro lado. El nombre pasa por `valid_filename`, igual que en `put`.

En el cliente saqué `put` y `get` a dos funciones, `do_put` y `do_get`, porque el bucle principal se estaba haciendo largo. También arreglé lo de las líneas largas: si no cabe la línea, se avisa y se descarta lo que sobra para que no se lea como otro comando.

Pruebas en la VM, todo bien a la primera:

- Subí con un cliente cinco archivos (texto, vacío, 3 MB, 70000 ceros y 50 MB) y los bajé con otro cliente distinto, con su propio `client_files`. Los cinco tienen el mismo md5 y tamaño.
- `get` de un archivo que no existe, de `../datos/texto.txt`, de `a/b` y de `.`: el cliente dice que no existe y no se crea nada.
- Una carpeta y un enlace simbólico dentro de `files/`: tampoco se bajan.
- Una línea de 5000 caracteres: el cliente avisa y el comando que sigue funciona normal.
- Ocho clientes bajando a la vez el mismo archivo de 3 MB: los ocho iguales.
- Subir un `texto.txt` nuevo y bajarlo enseguida devuelve la versión nueva.

Lo que falta es lo más delicado: SIGINT y SIGTERM, el sufijo para nombres repetidos y qué pasa si alguien se queda colgado a mitad de una subida.
