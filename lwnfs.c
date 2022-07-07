/*
 * Implementación del mkdir, touch y rmdir para lwnfs
 * Proyecto final de Sistemas Operations - UTEC
 * Integrantes: Morales Panitz, Alexander
 *              Ugarte Quispe, Grover
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pagemap.h> 	/* PAGE_CACHE_SIZE */
#include <linux/fs.h>     	/* This is where libfs stuff is declared */
#include <asm/atomic.h>
#include <asm/uaccess.h>	/* copy_to_user */
/*
 * Boilerplate stuff.
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jonathan Corbet");

#define LFS_MAGIC 0x19980122

/* Se declararon las variables globales al inicio del código
   para evitar problemas de variables no definidas.*/

// Se mantuvo la variable global counter.
static atomic_t counter, subcounter;
// Se declararon las variables globales para el manejo de archivos
struct inode_operations lwnfs_dir_inode_operations;

/*
 * Anytime we make a file or directory in our filesystem we need to
 * come up with an inode to represent it internally.  This is
 * the function that does that job.  All that's really interesting
 * is the "mode" parameter, which says whether this is a directory
 * or file, and gives the permissions.
 */

static struct inode *lfs_make_inode(struct super_block *sb, int mode)
{
	struct inode *ret = new_inode(sb);
    // Se especifició el kernel_time para el tiempo de creacion del inode.
	struct timespec64 kernel_time;
	if (ret) {
		ret->i_mode = mode;
        // Cambio de sintaxis por versiones de Linux Kernel
		ret->i_uid.val = ret->i_gid.val = 0;
		ret->i_size = PAGE_SIZE;
		ret->i_blocks = 0;
		ret->i_atime = ret->i_mtime = ret->i_ctime = kernel_time;
	}
	return ret;
}

/*
 * The operations on our "files".
 */

/*
 * Open a file.  All we have to do here is to copy over a
 * copy of the counter pointer so it's easier to get at.
 */
static int lfs_open(struct inode *inode, struct file *filp)
{
    // Cambio de sintaxis por versiones de Linux Kernel
	filp->private_data = inode->i_private;
	return 0;
}

#define TMPSIZE 20
/*
 * Read a file.  Here we increment and read the counter, then pass it
 * back to the caller.  The increment only happens if the read is done
 * at the beginning of the file (offset = 0); otherwise we end up counting
 * by twos.
 */
static ssize_t lfs_read_file(struct file *filp, char *buf,
		size_t count, loff_t *offset)
{
	atomic_t *counter = (atomic_t *) filp->private_data;
	int v, len;
	char tmp[TMPSIZE];
/*
 * Encode the value, and figure out how much of it we can pass back.
 */
	v = atomic_read(counter);
	if (*offset > 0)
		v -= 1;  /* the value returned when offset was zero */
	else
		atomic_inc(counter);
	len = snprintf(tmp, TMPSIZE, "%d\n", v);
	if (*offset > len)
		return 0;
	if (count > len - *offset)
		count = len - *offset;
/*
 * Copy it back, increment the offset, and we're done.
 */
	if (copy_to_user(buf, tmp + *offset, count))
		return -EFAULT;
	*offset += count;
	return count;
}

/*
 * Write a file.
 */
static ssize_t lfs_write_file(struct file *filp, const char *buf,
		size_t count, loff_t *offset)
{
	atomic_t *counter = (atomic_t *) filp->private_data;
	char tmp[TMPSIZE];
/*
 * Only write from the beginning.
 */
	if (*offset != 0)
		return -EINVAL;
/*
 * Read the value from the user.
 */
	if (count >= TMPSIZE)
		return -EINVAL;
	memset(tmp, 0, TMPSIZE);
	if (copy_from_user(tmp, buf, count))
		return -EFAULT;
/*
 * Store it in the counter and we are done.
 */
	atomic_set(counter, simple_strtol(tmp, NULL, 10));
	return count;
}


/*
 * Now we can put together our file operations structure.
 */
static struct file_operations lfs_file_ops = {
	.open	= lfs_open,
	.read 	= lfs_read_file,
	.write  = lfs_write_file,
};


/*
 * Create a file mapping a name to a counter.
 */
static struct dentry *lfs_create_file (struct super_block *sb,
		struct dentry *dir, const char *name,
		atomic_t *counter)
{
	struct dentry *dentry;
	struct inode *inode;
    /* En la versión de linux kernel definida para el proyecto existe una función llamada d_alloc_name que permite crear una entrada de directorio con un nombre dado. Para la compilación del código base, este fue el problema principal */
/*
 * Now we can create our dentry and the inode to go with it.
 */
    /* Se asigna el nombre al directory entry */
	dentry = d_alloc_name(dir, name);
	if (! dentry)
		goto out;
	inode = lfs_make_inode(sb, S_IFREG | 0644);
	if (! inode)
		goto out_dput;
	inode->i_fop = &lfs_file_ops;
    /* Cambio de sintaxis por versiones de Linux Kernel */
	inode->i_private = counter;
/*
 * Put it all into the dentry cache and we're done.
 */
	d_add(dentry, inode);
	return dentry;
/*
 * Then again, maybe it didn't work.
 */
  out_dput:
	dput(dentry);
  out:
	return 0;
}


/*
 * Create a directory which can be used to hold files.  This code is
 * almost identical to the "create file" logic, except that we create
 * the inode with a different mode, and use the libfs "simple" operations.
 */
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
/* A continuación, la implementación propia para el funcionamiento de
   la syscall mkdir y touch. */
 
/* Se procede a declarar la función de simple_mknod para la creacion de nodos
   en nuestro filesystem. */

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

/* Se procede a declarar la función de simple_create para la creacion de nodos en nuestro filesystem. */

static int simple_create(struct inode *inode, struct dentry *dentry, umode_t mode, bool excl)
{
    /* Simplemente realizamos el llamado a la funcion simple_mknod para
       la creacion del nodo */
	return simple_mknod(inode, dentry, mode, 0);
}

/* Se procede a declarar la función de simple_mkdir para la creacion de directorios en nuestro filesystem. */
static int simple_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	dentry = lfs_create_dir(dentry->d_sb, dentry->d_parent, dentry->d_name.name);
	return 0;
}

/* Tal como se había mencionado anteriormente, se require una propia
    implementacion de las funciones mkdir y touch. Por ello, definimos
    las funciones implementadas en la variable global */

/* IMPORTANTE: El linux kernel provee funciones simples para
   las demas operaciones. Por esa razón, se hace el llamado a <linux/fs.h> 
*/

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


/*
 * OK, create the files that we export.
 */

static void lfs_create_files (struct super_block *sb, struct dentry *root)
{
	struct dentry *subdir;
/*
 * One counter in the top-level directory.
 */
	atomic_set(&counter, 0);
	lfs_create_file(sb, root, "counter", &counter);
/*
 * And one in a subdirectory.
 */
	atomic_set(&subcounter, 0);
	subdir = lfs_create_dir(sb, root, "subdir");
	if (subdir)
		lfs_create_file(sb, subdir, "subcounter", &subcounter);
}


/*
 * Superblock stuff.  This is all boilerplate to give the vfs something
 * that looks like a filesystem to work with.
 */

/*
 * Our superblock operations, both of which are generic kernel ops
 * that we don't have to write ourselves.
 */
static struct super_operations lfs_s_ops = {
	.statfs		= simple_statfs,
	.drop_inode	= generic_delete_inode,
};

/*
 * "Fill" a superblock with mundane stuff.
 */
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


/*
 * Stuff to pass in when registering the filesystem.
 */
/* Cambio por la version de Linux Kernel */
static struct dentry *lfs_get_super(struct file_system_type *fst,
		int flags, const char *devname, void *data)
{
    /* Cambio por la version de Linux Kernel */
	return mount_bdev(fst, flags, devname, data, lfs_fill_super);
}

static struct file_system_type lfs_type = {
	.owner 		= THIS_MODULE,
	.name		= "lwnfs",
    /* Cambio por la version de Linux Kernel */
	.mount		= lfs_get_super,
	.kill_sb	= kill_litter_super,
};


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
