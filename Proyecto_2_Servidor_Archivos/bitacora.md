# Bitácora del proyecto 2: servidor de archivos

Edward Esteban Dávila Salazar y Miguel Angel Perez Mera

## 20 de octubre

Leímos el PDF y el zip del profe. El zip trae esqueletos: el servidor atiende a un solo cliente y con el puerto fijo en 1234, el cliente apenas se conecta, y `send_file` y `receive_file` son puros comentarios. Lo que sí viene hecho es `split`, `util` y `netutil`.

Lo que hay que lograr, resumido: un servidor y un cliente, carpeta `files/` en el servidor y `client_files/` en el cliente (las dos se crean si no existen), comandos `put`, `get` y `exit`, un hilo por conexión con el arreglo de clientes protegido por un mutex, y qué hacer si dos clientes mandan el mismo nombre a la vez. También terminar bien con SIGINT y SIGTERM, y entregar un zip con un README.txt con nombres, códigos y correos.

Para los nombres repetidos nos quedamos con el sufijo (`_1`, `_2`) y no con rechazar. Un `put` suelto de un nombre que ya existe sobrescribe, como dice el PDF.

Mucho se parece al chat del taller 6 (hilo por cliente, lista con mutex) y a lo de señales del taller 5, así que partimos de ahí. Lo nuevo de verdad es el protocolo de `put` y `get`.

Dividimos el trabajo en cinco commits: la base, `put`, `get`, señales y concurrencia, y la entrega.

## 22 de octubre

Nota del jueves: el servidor de archivos está anónimo. Cualquier cliente puede sobrescribir los archivos de otro. Hay que separar los archivos por cliente.

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

## 26 de octubre, cuarto commit

Este fue el más enredado: señales, nombres repetidos y los casos donde alguien se queda colgado.

Señales. Quise evitar el problema que me quedó pendiente en el taller 5, que una señal llegue justo entre revisar la bandera y entrar a esperar. En el servidor bloqueo SIGINT y SIGTERM desde el principio, así los hilos las heredan bloqueadas y solo `main` las recibe, y espera conexiones con `ppoll`, que las desbloquea solo mientras espera. El manejador solo pone `finished` en 1. Al terminar, el servidor cierra el socket de escucha, hace `shutdown` a los sockets de los clientes para despertar a los hilos que están en un `read`, y espera hasta 5 segundos a que se vayan todos (con una variable de condición, porque los hilos son `detached` y no se les puede hacer `join`). `ppoll` pide `_GNU_SOURCE`, que no estaba y por eso no compilaba al principio.

En el cliente el manejador pone una bandera y también `protocol_abort`, que está en `protocol.c`. Sin eso `read_all` seguía reintentando cuando una señal lo interrumpía y el cliente no salía hasta que llegaba la respuesta.

Nombres repetidos. Hay una tabla de nombres que se están recibiendo, protegida con otro mutex. Si el nombre ya está ahí, se le pone sufijo antes de la extensión (`dup.txt` pasa a `dup_1.txt`), y también se salta los sufijos que ya existen en disco para no pisar archivos viejos. El nombre original sí puede sobrescribir uno que ya está guardado, como pide el PDF. El servidor le dice al cliente cómo lo va a guardar.

Para que nadie vea archivos a medias, `receive_file` ahora escribe en `.nombre.part` y al final hace `rename`. Por eso `valid_filename` rechaza todo lo que empiece por punto, para que un cliente no pueda usar esos nombres. Una consecuencia buena: `rename` reemplaza un enlace simbólico en vez de seguirlo.

Si un cliente deja de mandar datos en una subida más de 30 segundos, el servidor lo corta (`SO_RCVTIMEO`). También agregué una pausa corta cuando `accept` falla por algo que no se arregla solo, para no girar en vacío.

Pruebas en la VM:

- Cuatro clientes subiendo a la vez el mismo nombre: la primera prueba no sirvió, porque los archivos eran tan rápidos de copiar que casi no se cruzaban. La repetí dejando tres subidas atascadas con el mismo nombre (con un programa de prueba que arma los mensajes a mano) y luego una subida real. Se reservaron `dup2.txt`, `dup2_1.txt` y `dup2_2.txt`, y la real quedó como `dup2_3.txt`.
- Con `dup3_1.txt` ya en disco, la subida real quedó como `dup3_2.txt` y el viejo siguió intacto. Sin extensión, `notas` quedó `notas_1`.
- Subir de nuevo un nombre que no está en uso sobrescribe, sin sufijo.
- Nombres como `.oculto` y `.dup.txt.part` se rechazan.
- SIGTERM al servidor con dos clientes conectados: cerró con código 0 y los dos clientes se despidieron. SIGINT con una subida detenida a la mitad: terminó con código 0, el `.part` desapareció y no quedó nada.
- Cliente con SIGINT en el prompt: imprime "Interrumpido" y sale con 0. Cliente con SIGTERM mientras bajaba de un servidor falso que se atasca: salió al instante y no quedó ningún archivo en `client_files`.
- Subida atascada sin señales: el servidor cortó la conexión a los 32 segundos y no dejó temporales.
- Repetí las pruebas de `put` y `get` del commit anterior contra esta versión: todo igual, md5 de 50 MB incluido.

Un error mío en las pruebas: probé a un cliente desde una carpeta que no tenía el ejecutable, y la salida salió vacía. No era un bug del programa.

Sin probar: señales con muchos clientes subiendo al mismo tiempo, y qué pasa si el disco se llena a mitad de una subida. Falta el último commit: Doxygen, comentarios, el README.txt para el zip y una pasada final.

## 26 de octubre, quinto commit

Para cerrar pasé el código por sanitizadores, y después hice la documentación.

Corrí el servidor y seis clientes con AddressSanitizer y UBSan, y otra vez con ThreadSanitizer, con una carga mezclada: cada cliente hacía varios `put` de archivos grandes con los mismos nombres (así se cruzaban las reservas con sufijo), `get` de lo que acababa de subir, un `get` que no existe, un `put` de algo inexistente, una palabra suelta, una línea vacía y un `get ../x`. Al final, con un cliente conectado y quieto, mandé SIGTERM al servidor. Los dos sanitizadores salieron sin ningún aviso, ni del servidor ni de los clientes. Quedaron `a.bin` más cinco con sufijo, `b.bin` más tres, `c.txt` más cuatro o cinco, y ningún `.part`.

Lo que no cubre eso: los sanitizadores solo ven lo que la carga alcanza a ejecutar. No probé disco lleno y no probé más de seis clientes con ellos puestos.

Documentación: agregué `Doxyfile` y `make doc`. Doxygen sale sin advertencias. Las funciones propias ya traían su comentario Doxygen en los prototipos, y le puse el encabezado `@file` a `protocol.h` y `protocol.c`. No toqué los archivos del profe, que siguen idénticos al zip.

Lo que quedó como está y vale saberlo:

- Si se llenan los 256 lugares con conexiones quietas, nadie más entra. Un cliente que solo se queda conectado no se corta, porque el PDF no dice nada de eso. El límite de espera de 30 segundos es solo para subidas.
- El servidor escucha en todas las interfaces y no pide ninguna clave.
- `split` del profe sigue sin controlar el límite de palabras. El cliente lo cubre rechazando antes la línea.
- El tamaño anunciado en una subida no se limita. Si alguien declara varios GB, el servidor empieza a escribir hasta que se acabe el disco o el cliente deje de mandar.

El zip para el profe lleva los `.c`, los `.h`, el `Makefile` y un `README.txt` con los nombres, códigos y correos. La bitácora y el `Doxyfile` quedan solo en el repositorio.

## 28 de octubre, list y carpetas por usuario

Retomé la nota del jueves 22 y lo que dijo el profe de implementar `list`.

El servidor ya no es anónimo. Lo primero que hace el cliente al conectarse es decir su usuario (`./client 127.0.0.1 1234 ana`, o lo pide por teclado), y el servidor crea `files/ana/`. Todo lo que ese cliente sube, baja o lista pasa por esa carpeta. Sin usuario, el servidor contesta "primero hay que identificarse" a todo menos a `exit`. Un usuario solo se puede declarar una vez por conexión, y el nombre tiene que ser de 1 a 32 letras, números, `_`, `-` o `.`, sin empezar por punto, así que no sirve para salirse de `files/`.

Esto es identificar, no autenticar: no hay clave. Cualquiera puede escribir "ana" y ver los archivos de ana. Lo que sí cambia es que ya no se pisan por accidente, y que un cliente que no conozca el nombre de otro no llega a sus archivos. Si el profe pide algo más fuerte habría que agregar una clave por usuario, y eso no lo hice.

`list` manda un `file_info` por cada archivo regular de la carpeta del usuario y uno final con tamaño -1 para cerrar la lista, la misma idea del -1 que ya usaba `get`. No muestra carpetas, enlaces, archivos que empiezan por punto ni temporales, y los manda ordenados.

La reserva de nombres con sufijo ahora es por usuario: la tabla guarda `usuario/nombre`. Dos usuarios pueden subir a la vez un `notas.txt` sin estorbarse, y el sufijo solo aparece si es el mismo usuario con dos subidas del mismo nombre.

Lo que falló: la primera corrida de pruebas contestó "ya está identificado" a todos. El arreglo `clients` reutiliza posiciones y yo no borraba el usuario del cliente anterior, así que el siguiente que caía en esa posición heredaba el usuario. Lo arreglé en `add_client`. También salieron dos advertencias de `snprintf` por posible truncado.

Pruebas en la VM:

- Nombres de usuario inválidos (`../etc`, `.oculto`, `a/b`, uno con espacio, uno vacío y uno de más de 32 letras): todos rechazados. Una segunda identificación en la misma conexión: rechazada.
- `put`, `list` y `get` sin identificarse: los tres contestan que primero hay que identificarse.
- Ana y beto suben un `notas.txt` con contenido distinto: quedan en `files/ana/` y `files/beto/` sin mezclarse. Beto intenta bajar `grande.bin` de ana y el servidor dice que no existe. Cuando beto sube otra vez su `notas.txt`, el de ana no cambia.
- `list`: una carpeta vacía dice "No hay archivos". Con un directorio, un enlace simbólico, un archivo oculto y un temporal dentro de la carpeta de ana, solo salen sus dos archivos de verdad. Con 400 archivos salieron las 400 líneas.
- Ana con una subida atascada de `mismo.txt` y beto subiendo `mismo.txt`: beto lo guarda con su nombre, sin sufijo. Ana subiendo otra vez el suyo: `mismo_1.txt`.
- Repetí los sanitizadores con la carga mezclada, ahora con tres usuarios, y salieron sin avisos ni AddressSanitizer ni ThreadSanitizer. Doxygen sigue sin advertencias.

Sin probar: muchos usuarios distintos a la vez (probé tres), y qué pasa si dos conexiones del mismo usuario hacen `get` de un archivo mientras otra lo sobrescribe. Con el `rename` el archivo nunca está a medias, pero un `get` que ya estaba mandando uno viejo seguiría con el viejo.

## 28 de octubre, clave por usuario y pruebas que faltaban

Antes dejé dicho que cualquiera podía escribir "ana" y ver los archivos de ana. Hoy le puse clave.

La primera vez que un usuario entra, la clave que escribe queda como su clave. Después hay que ponerla igual. Se guarda en `files/USUARIO/.clave` con `crypt` (SHA-512 y una sal al azar), con permisos 600, y como empieza por punto ni `list` ni `get` la tocan. El cliente la pide sin mostrarla si la entrada es una terminal. Son de 1 a 64 caracteres y a la tercera mala el servidor corta la conexión. Se manda por el socket sin cifrar, eso no lo arreglé.

El protocolo cambió un poco: `user` solo guarda el nombre y falta mandar `pass`. La carpeta del usuario se crea al poner la clave, no antes, para que quien solo prueba nombres no deje carpetas vacías. El Makefile ahora enlaza el servidor con `-lcrypt`.

Si dos clientes crean a la vez el mismo usuario nuevo, la clave se escribe en un temporal y se enlaza a `.clave` con `link`, que falla si ya existe. El que pierde compara contra la del ganador.

También puse un tope de 1 GiB por archivo. El cliente lo revisa antes de mandar nada y el servidor lo revisa en `receive_file`, por si alguien declara un tamaño mayor.

Lo que probé en la VM:

- Clave mala tres veces: corta. Clave buena después: entra. Beto con otra clave no ve ni baja lo de ana. Clave vacía y clave de 65 caracteres: rechazadas.
- Mandando los mensajes a mano con un script de Python: `list`, `put`, `get` y `pass` sin identificarse dan error, y `list` justo después de `user` también, porque falta la clave.
- Archivo de 1 GiB más un byte: el cliente no lo manda. Declarando 2 GiB desde el script, el servidor contesta que no se pudo recibir y sigue vivo.
- 40 usuarios a la vez, cada uno con un put, un get y un list: los 40 archivos quedaron iguales en el servidor y al bajarlos (md5).
- 10 clientes creando el mismo usuario nuevo con claves distintas: solo entró uno, y después solo sirve esa clave.
- Un get repetido 12 veces mientras otro cliente sobrescribía el mismo archivo con dos versiones de 20 MB: los 12 llegaron completos y de una sola versión, ninguno mezclado.
- Disco lleno, con un tmpfs de 3 MB y una subida de 6 MB: el servidor contesta que no pudo recibir, no queda ningún `.part` y otro cliente sube un archivo chico sin problema. Con 2 MB ni siquiera se pudo crear la carpeta del usuario nuevo y el login dio "error del servidor".
- ASan con UBSan y TSan con la carga de los 40 usuarios: sin avisos. Doxygen sin advertencias.

Un bug que no tenía que ver con la clave: al compilar con `-O2` salió que `alive` en `main` del cliente podía quedar sin valor cuando hacía `put` o `get` sin nombre de archivo, o con uno que no existe. Se ve el "Uso:" y después `if (!alive)` leía basura. Ya lo inicializo antes del `if`.

Errores míos en las pruebas, no del programa: la primera prueba de disco lleno falló porque el archivo de 6 MB lo generé dentro del mismo disco de 2 MB, y el script de los 40 usuarios se quedó colgado porque su `wait` esperaba también al servidor.

Lo que sigue sin resolver: 256 conexiones inactivas llenan el servidor, escucha en todas las interfaces, y las tres claves malas son por conexión, así que alguien puede volver a conectarse y seguir probando.
