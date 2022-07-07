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

static ssize_t lfs_write_file(struct file *filp, const char *buf,
		size_t count, loff_t *offset)
static struct dentry *lfs_create_file (struct super_block *sb,
		struct dentry *dir, const char *name,
		atomic_t *counter)
static struct dentry *lfs_create_dir (struct super_block *sb,
		struct dentry *parent, const char *name)
static int simple_mknod(struct inode *dir, struct dentry *dentry, umode_t mode, dev_t dev)
static int simple_create(struct inode *inode, struct dentry *dentry, umode_t mode, bool excl)
static int simple_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
static void lfs_create_files (struct super_block *sb, struct dentry *root)

static int lfs_fill_super (struct super_block *sb, void *data, int silent)
static struct dentry *lfs_get_super(struct file_system_type *fst,
		int flags, const char *devname, void *data)
static int __init lfs_init(void)
static void __exit lfs_exit(void)
```

## Creación de un nuevo nodo

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
## Función para abrir un archivo
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
