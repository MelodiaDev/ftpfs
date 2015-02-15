#include "ftpfs.h"
#include "super.h"
#include "inode.h"
#include "file.h"


int __init ftpfs_init(void) {
    pr_debug("ftpfs module loaded\n");

    register_filesystem(&ftp_fs_type);
    return 0;
}

void __exit ftpfs_fini(void) {
    pr_debug("ftpfs module unloaded\n");

    unregister_filesystem(&ftp_fs_type);
}

module_init(ftpfs_init); // Maybe fs_initcall() is more appropriate
module_exit(ftpfs_fini);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("vani");
