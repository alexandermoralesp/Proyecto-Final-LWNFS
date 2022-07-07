# LWNFS - OS UTEC

Para la instalación, primero configurar el mount.sh. IMPORTANTE

```
$ sudo fdisk -l # Verificar el primer loop device activo -> Ejemplo /dev/loop11
```

Cambiar en mount.sh por
```
sudo mount -t lwnfs <tu loop device activo> ./mount_point
```

Ejecutar
```
$ sudo bash umount.sh # para compilar y desmontar
$ sudo bash mount.sh # para montar
```

```
$ cd ./mount_point
$ mkdir newfiles
$ mkdir newfile
$ cat newfile <- retorna contador
```
