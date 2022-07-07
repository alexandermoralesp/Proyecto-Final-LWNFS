mkidr mount_point
sudo umount mount_point
sudo rmmod lwnfs
sudo make clean
sudo make
sudo insmod lwnfs.ko
