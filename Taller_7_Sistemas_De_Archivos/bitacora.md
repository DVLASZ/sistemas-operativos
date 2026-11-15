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

