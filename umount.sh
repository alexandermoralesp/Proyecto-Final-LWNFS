sudo umount mnt
sudo rmmod lwnfs
sudo make clean
sudo make
sudo insmod lwnfs.ko
