obj-m := ftpfs.o
ftpfs-objs := ftpfs.o

CFLAGS_ftpfs.o = -DDEBUG

KDIR ?= /lib/modules/`uname -r`/build

all:
	make -C $(KDIR) M=$$PWD modules
clean:
	make -C $(KDIR) M=$$PWD clean
