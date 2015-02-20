Ftpfs
==============
FTP file system

Usage
==============
# build the kernel
make 
# load the module into kernel
sudo insmod ftpfs.ko 
# mount this filesystem
sudo mount -t ftpfs none /mnt 
# ls command
sudo ls /mnt 
# read a file
sudo cat /mnt/ftp/test.txt 
# write a file
sudo echo "hello world!" | sudo tee /mnt/ftp/test.txt 

