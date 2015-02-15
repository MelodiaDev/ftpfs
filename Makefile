obj-m := ftpfs.o
ftpfs-objs := init.o inode.o super.o file.o sock.o ftp.o

CFLAGS_init.o = -DDEBUG
CFLAGS_inode.o = -DDEBUG
CFLAGS_super.o = -DDEBUG
CFLAGS_file.o = -DDEBUG
CFLAGS_sock.o = -DDEBUG
CFLAGS_ftp.o = -DDEBUG

KDIR ?= /lib/modules/`uname -r`/build

all:
	make -C $(KDIR) M=$$PWD modules
clean:
	make -C $(KDIR) M=$$PWD clean
