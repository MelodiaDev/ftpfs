#include <linux/module.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/highmem.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/backing-dev.h>
#include <linux/ramfs.h>
#include <linux/sched.h>
#include <linux/parser.h>
#include <linux/magic.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include "ftpfs.h"
#include "super.h"
#include "inode.h"


int __init ftpfs_init(void) {
    static unsigned long once;
    int err;

    if (test_and_set_bit(0, &once)) return 0; // avoiding race condition
    pr_debug("ftpfs module loaded\n");
    
    if ((err = bdi_init(&ftp_fs_bdi)))
        return err;

    if ((err = register_filesystem(&ftp_fs_type)))
        bdi_destroy(&ftp_fs_bdi);
    return 0;
}

void __exit ftpfs_fini(void) {
    pr_debug("ftpfs module unloaded\n");
}

module_init(ftpfs_init); // Maybe fs_initcall() is more appropriate
module_exit(ftpfs_fini);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("vani");
