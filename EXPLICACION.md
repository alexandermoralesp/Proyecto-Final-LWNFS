# Explicacion del Source Code

En el presente código, procederes a analizar el código base de lwnfc y nuestra implementación


Como en todo programa, primero declaramos nuestras variables y funciones a utilizar
```c

// Se mantuvo la variable global counter.
static atomic_t counter, subcounter;
// Se declararon las variables globales para el manejo de archivos
struct inode_operations lwnfs_dir_inode_operations;

// Función general para crear un nodo
static struct inode *lfs_make_inode(struct super_block *sb, int mode);

//  Función para manejo de archivo open
static int lfs_open(struct inode *inode, struct file *filp);

// Función para manejo de archivo read
static ssize_t lfs_read_file(struct file *filp, char *buf,
		size_t count, loff_t *offset);

// Funcion para escribir en un archivo
static ssize_t lfs_write_file(struct file *filp, const char *buf,
		size_t count, loff_t *offset)
// FUnción para crear un archvio
static struct dentry *lfs_create_file (struct super_block *sb, struct dentry *dir, const char *name, atomic_t *counter)

// Función para crear un directorio lwnfs
static struct dentry *lfs_create_dir (struct super_block *sb, struct dentry *parent, const char *name)

// Sycall para crear un nodo
static int simple_mknod(struct inode *dir, struct dentry *dentry, umode_t mode, dev_t dev)

// Sycall para crear un archivo
static int simple_create(struct inode *inode, struct dentry *dentry, umode_t mode, bool excl)

// Sycall para crear un directorio
static int simple_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)

// Funcion de ejemplo para crear los archivos
static void lfs_create_files (struct super_block *sb, struct dentry *root)

// Funcion para crear el directorio raiz
static int lfs_fill_super (struct super_block *sb, void *data, int silent)

// Función para inicializar el sistema de archivos
static struct dentry *lfs_get_super(struct file_system_type *fst, int flags, const char *devname, void *data)

// Funcion para inicializar el sistema de archivos
static int __init lfs_init(void)

// Funcion para salir del sistema de archivos
static void __exit lfs_exit(void)
```

### Creación de un nuevo nodo

```c
static struct inode *lfs_make_inode(struct super_block *sb, int mode)
{
    // Hacemos llamado a la funcion new_inode para 
    // inicializar un nuevo nodo
	struct inode *ret = new_inode(sb);
    // Se especifició el kernel_time para el tiempo de creacion del inode.
	struct timespec64 kernel_time;
	
    if (ret) {
        // Definimos el modo
        // Recordar que IF_REG es para archivos, IF_DIR es 
        // para directorios
    	ret->i_mode = mode;
        // Cambio de sintaxis por versiones de Linux Kernel
		ret->i_uid.val = ret->i_gid.val = 0;
        // Tamaño del nodo
		ret->i_size = PAGE_SIZE;
        // Cantidad e bloques <- BTree
		ret->i_blocks = 0;
        // Añadimos el tiempo al inode
		ret->i_atime = ret->i_mtime = ret->i_ctime = kernel_time;
	}
	return ret;
}
```

### Función para abrir un archivo
```c
static int lfs_open(struct inode *inode, struct file *filp)
{
    // Cambio de sintaxis por versiones de Linux Kernel
    // Para cada archivo se extra el contador
    // la cual está almacenada en inode->i_private
	filp->private_data = inode->i_private;
	return 0;
}
```

### Función para leer un archivo

```c
static ssize_t lfs_read_file(struct file *filp, char *buf,
		size_t count, loff_t *offset)
{
    // Extraeemos el contador
	atomic_t *counter = (atomic_t *) filp->private_data;
	// Definimos las variables globales
    int v, len;
	char tmp[TMPSIZE];
    // Leemos atomico <- Es un variable atomica
	v = atomic_read(counter);
	if (*offset > 0)
		v -= 1; // el valor devuelto cuando el desplazamiento era cero
	else
		atomic_inc(counter);
	len = snprintf(tmp, TMPSIZE, "%d\n", v);
	if (*offset > len)
		return 0;
	if (count > len - *offset)
		count = len - *offset;

	// Copiamos e incrementalos
	if (copy_to_user(buf, tmp + *offset, count))
		return -EFAULT;
	*offset += count;
	return count;
}
```

### Función para escribir un archivo
```c
/*
 * Write a file.
 */
static ssize_t lfs_write_file(struct file *filp, const char *buf,
		size_t count, loff_t *offset)
{
	// Extramos el contador del archivo
	atomic_t *counter = (atomic_t *) filp->private_data;
	char tmp[TMPSIZE];

	// Solo se puede escribir desde el inicio
	if (*offset != 0)
		return -EINVAL;
	// Leer el valor por el usuario
	if (count >= TMPSIZE)
		return -EINVAL;
	memset(tmp, 0, TMPSIZE);
	if (copy_from_user(tmp, buf, count))
		return -EFAULT;
	// Guarar la informacion en el contador
	atomic_set(counter, simple_strtol(tmp, NULL, 10));
	return count;
}
```

### Operaciones de un archivo

Definimos las syscalls generales para el tratamiento de archivos. Esto será incluido en inode->i_fops.
```c
static struct file_operations lfs_file_ops = {
	.open	= lfs_open,
	.read 	= lfs_read_file,
	.write  = lfs_write_file,
};
```

### Función general para crear un archivo
```c
static struct dentry *lfs_create_file (struct super_block *sb,
		struct dentry *dir, const char *name,
		atomic_t *counter)
{
	struct dentry *dentry;
	struct inode *inode;
    /* En la versión de linux kernel definida para el proyecto existe una función llamada d_alloc_name que permite crear una entrada de directorio con un nombre dado. Para la compilación del código base, este fue el problema principal */
	// Creamos el directorio y el inodo para el archivo
    /* Se asigna el nombre al directory entry */
	dentry = d_alloc_name(dir, name);
	if (! dentry)
		goto out;
	// S_IFREG es para archivos
	inode = lfs_make_inode(sb, S_IFREG | 0644);
	if (! inode)
		goto out_dput;
	inode->i_fop = &lfs_file_ops;
    /* Cambio de sintaxis por versiones de Linux Kernel */
	inode->i_private = counter;
	// Guardamos el inodo en el directorio
	d_add(dentry, inode);
	return dentry;
  out_dput:
	dput(dentry);
  out:
	return 0;
}
```

### Función general para crear un directorio
La función es similar a la de la creación de archivos. La diferencia es que el mode cambia a S_IFDIR.
```c
static struct dentry *lfs_create_dir (struct super_block *sb,
		struct dentry *parent, const char *name)
{
	struct dentry *dentry;
	struct inode *inode;
    /* En la versión de linux kernel definida para el proyecto existe una función llamada d_alloc_name que permite crear una entrada de directorio con un nombre dado. Para la compilación del código base, este fue el problema principal */


    /* Se asigna el nombre al directory entry */
	dentry = d_alloc_name(parent, name);
	if (! dentry)
		goto out;

	inode = lfs_make_inode(sb, S_IFDIR | 0644);
	if (! inode)
		goto out_dput;
    /* IMPORTANTE: Se cambió las operaciones por default para la syscall de mkdir por las operaciones definidas en lwfs. Esto permita que se pueda realizar el llamda mkdir en cualquier directorio padre*/
	inode->i_op = &lwnfs_dir_inode_operations;
	inode->i_fop = &simple_dir_operations;

	d_add(dentry, inode);
	return dentry;

  out_dput:
	dput(dentry);
  out:
	return 0;
}
```
## Funciones implementadas -->
### Funcion para crear un nodo
```c
static int simple_mknod(struct inode *dir, struct dentry *dentry, umode_t mode, dev_t dev)
{
    /* Ya que la función proporcionada por lwnfs para la creacion
       de archivos se requirere el contador. Para este caso usamos la función atomic_inc() vista en el curso para realizar el incremento cada vez que se cree un nuevo nodo */
    // Incrementamos el contador  
	atomic_inc(&counter);
    // Realizamos el llamado a la funcion create_file
    dentry = lfs_create_file(dentry->d_sb, dentry->d_parent, dentry->d_name.name, &counter);
	return 0;
}
```
### Operaciones de directorio

```c
static int simple_create(struct inode *inode, struct dentry *dentry, umode_t mode, bool excl)
{
    /* Simplemente realizamos el llamado a la funcion simple_mknod para
       la creacion del nodo */
	return simple_mknod(inode, dentry, mode, 0);
}
```
### Funcion general para crear archivos

```c
static int simple_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	dentry = lfs_create_dir(dentry->d_sb, dentry->d_parent, dentry->d_name.name);
	return 0;
}
```

### Operaciones para el root principal

```c
struct inode_operations lwnfs_dir_inode_operations = {
    // Definimos la funcion de creacion de archivos
	.create = simple_create,
	.lookup = simple_lookup,
	.link = simple_link,
	.unlink = simple_unlink,
    // Definimos la funcion de creacion de directorios
	.mkdir = simple_mkdir,
	.rmdir = simple_rmdir,
    // Definimos la funcion de creacion de nodos
	.mknod = simple_mknod,
};

```
### Funcion de inicialización del super block y del directorio raiz

```c
static void lfs_create_files (struct super_block *sb, struct dentry *root)
{
	struct dentry *subdir;
	// Setea como contador a 0
	atomic_set(&counter, 0);
	lfs_create_file(sb, root, "counter", &counter);
	// Setea como subcontador a 0
	atomic_set(&subcounter, 0);
	subdir = lfs_create_dir(sb, root, "subdir");
	if (subdir)
		lfs_create_file(sb, subdir, "subcounter", &subcounter);
}
```
### Funciones generales de inicializacion
Se declara las operaciones para el super_block
```c
static struct super_operations lfs_s_ops = {
	.statfs		= simple_statfs,
	.drop_inode	= generic_delete_inode,
};

```
### Registro del file system
Dicha función se encarga de inicializar los parametros del file system.
```c
static int lfs_fill_super (struct super_block *sb, void *data, int silent)
{
	struct inode *root;
	struct dentry *root_dentry;
/*
 * Basic parameters.
 */
    /* Cambio por la version de Linux Kernel */
	sb->s_blocksize = PAGE_SIZE;
	sb->s_blocksize_bits = PAGE_SHIFT;
	sb->s_magic = LFS_MAGIC;
	sb->s_op = &lfs_s_ops;
/*
 * We need to conjure up an inode to represent the root directory
 * of this filesystem.  Its operations all come from libfs, so we
 * don't have to mess with actually *doing* things inside this
 * directory.
 */
	root = lfs_make_inode (sb, S_IFDIR | 0755);
	if (! root)
		goto out;
    /* Definimos las operaciones de lwnfs para el directorio raiz */
	root->i_op = &lwnfs_dir_inode_operations;
	root->i_fop = &simple_dir_operations;
/*
 * Get a dentry to represent the directory in core.
 */
    /* Cambio por la version de Linux Kernel */
	root_dentry = d_make_root(root);
	if (! root_dentry)
		goto out_iput;
	sb->s_root = root_dentry;
/*
 * Make up the files which will be in this filesystem, and we're done.
 */
	lfs_create_files (sb, root_dentry);
	return 0;
	
  out_iput:
	iput(root);
  out:
	return -ENOMEM;
}

```

### Funciones finales
Registro del file system, dar de baja el filesystem.
```c
/*
 * Get things set up.
 */
static int __init lfs_init(void)
{
	return register_filesystem(&lfs_type);
}

static void __exit lfs_exit(void)
{
	unregister_filesystem(&lfs_type);
}

module_init(lfs_init);
module_exit(lfs_exit);
```
