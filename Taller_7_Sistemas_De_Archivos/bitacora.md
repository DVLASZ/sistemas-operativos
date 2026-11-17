# Bitácora: taller de sistemas de archivos

Autores: Edward Esteban Dávila Salazar, Miguel Angel Perez Mera

Todo se probó en la máquina virtual Debian 13 del laboratorio, con gcc 14.2.0. Los archivos de prueba quedaron en el disco principal (ext4) y para el punto 7 usé `/tmp`, que es un tmpfs, o sea otro sistema de archivos.

## Primera parte

Los tres comandos del punto 1. En `df` dejé solo las filas que importan.

```
$ df -h
Filesystem      Size  Used Avail Use% Mounted on
/dev/sda1        38G  9.9G   26G  29% /
tmpfs           3.9G     0  3.9G   0% /tmp

$ df -i
Filesystem      Inodes   IUsed   IFree IUse% Mounted on
/dev/sda1      2485504  215572 2269932    9% /
tmpfs          1048576      20 1048556    1% /tmp

$ lsblk -f
NAME   FSTYPE FSVER LABEL UUID                                 FSAVAIL FSUSE% MOUNTPOINTS
sda
├─sda1 ext4   1.0         0fad2fdc-543d-47bc-9b4b-5187cabc16e8   25.2G    27% /
├─sda2
└─sda5 swap   1           6ef3be58-0cc0-4338-afda-620cf8f04100                [SWAP]
sr0
```

El disco real es `sda1`, ext4, con el 29% de espacio usado. Lo demás es tmpfs, que vive en memoria.

### Punto 2

`df -h` cuenta bloques de datos y `df -i` cuenta inodos, las estructuras de metadatos. Cada archivo, directorio o enlace gasta uno, y en ext4 la cantidad se fija al formatear. En mi disco hay 2 485 504 y van 215 572, un 9%, mientras que el espacio va en el 29%.

Un sistema de archivos queda inutilizable con espacio libre cuando se acaban los inodos. Pasa con millones de archivos muy pequeños, como la caché de un programa o las sesiones de un servidor. Sobran bloques, pero crear cualquier archivo falla con "No space left on device" y `df -h` sigue mostrando espacio.

### Punto 3

Creé `archivo.txt` con tres líneas (54 bytes) y le hice `stat`, a él y a su carpeta.

```
$ stat archivo.txt
  Size: 54        Blocks: 8          IO Block: 4096   regular file
Device: 8,1       Inode: 2354862     Links: 1
Access: (0664/-rw-rw-r--)  Uid: ( 1000/ ingesis)   Gid: ( 1000/ ingesis)

$ stat .
  Size: 4096      Blocks: 8          IO Block: 4096   directory
Device: 8,1       Inode: 2354861     Links: 2
Access: (0775/drwxrwxr-x)  Uid: ( 1000/ ingesis)   Gid: ( 1000/ ingesis)

$ ls -li
2354862 -rw-rw-r-- 1 ingesis ingesis 54 archivo.txt
```

El número del principio en `ls -li` es el inodo. El nombre `archivo.txt` está en el directorio, que es otro archivo (inodo 2354861) con una tabla de nombres e inodos.

## Segunda parte

Punto 4:

```
$ ln archivo.txt duro.txt
$ ln -s archivo.txt simbolico.txt
$ ls -li archivo.txt duro.txt simbolico.txt
2354862 -rw-rw-r-- 2 ingesis ingesis 54 archivo.txt
2354862 -rw-rw-r-- 2 ingesis ingesis 54 duro.txt
2354863 lrwxrwxrwx 1 ingesis ingesis 11 simbolico.txt -> archivo.txt
```

### Punto 5

| nombre | inodo | enlaces | tamaño |
|---|---|---|---|
| archivo.txt | 2354862 | 2 | 54 |
| duro.txt | 2354862 | 2 | 54 |
| simbolico.txt | 2354863 | 1 | 11 |

`archivo.txt` y `duro.txt` son dos nombres del mismo archivo: mismo inodo y contador en 2. El enlace simbólico es otro archivo, con su inodo y su contador en 1, y lo que guarda es el texto "archivo.txt". Por eso mide 11 y no 54.

Me equivoqué en la primera corrida. Había escrito "hola mundo" en el archivo, que con el salto de línea son 11 bytes, igual que las letras de "archivo.txt". Daban lo mismo y no se veía la diferencia. Repetí con tres líneas.

### Punto 6

```
$ rm archivo.txt
$ cat duro.txt
primera linea del archivo
segunda linea
tercera linea
$ cat simbolico.txt
cat: simbolico.txt: No such file or directory
```

`rm` borra un nombre, no el archivo: quita la entrada del directorio y baja el contador. Los datos se liberan cuando el contador llega a 0 y nadie los tiene abiertos. Aquí quedó en 1, así que `duro.txt` sigue igual, porque no era una copia sino otro nombre del mismo inodo.

El enlace simbólico busca el texto "archivo.txt" cada vez que se abre. Ese nombre ya no existe y por eso `cat` falla.

### Punto 7

```
$ ln duro.txt /tmp/duro_otro_fs.txt
ln: failed to create hard link '/tmp/duro_otro_fs.txt' => 'duro.txt': Invalid cross-device link
$ ln -s ~/prueba/duro.txt /tmp/simb_otro_fs.txt
$ cat /tmp/simb_otro_fs.txt
primera linea del archivo
segunda linea
tercera linea
```

Con `stat` vi que `duro.txt` está en el dispositivo 2049 (ext4) con inodo 2354862, y un archivo nuevo en `/tmp` quedó en el dispositivo 34 (tmpfs) con inodo 45.

Los números de inodo solo valen dentro de su sistema de archivos, porque cada uno cuenta por su lado. Un enlace duro es un nombre más un número de inodo, y en otro sistema ese número sería otro archivo o ninguno. Por eso `ln` lo rechaza.

El enlace simbólico guarda una ruta escrita, no un inodo. El kernel la recorre desde la raíz al abrirlo y esa ruta puede pasar por cualquier sistema montado.

## Tercera parte

Escribí `info_archivo.c` con `lstat`. Con `stat` un enlace simbólico se vería como el archivo al que apunta y no se podría ver su propio tamaño, que es justo el error que el taller marca. Compila sin advertencias con `gcc -Wall -Wextra`.

