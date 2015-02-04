#include <linux/init.h>
#include <linux/module.h>

int __init ftpfs_init(void) {
    pr_debug("ftpfs module loaded\n");
    return 0;
}

void __exit ftpfs_fini(void) {
    pr_debug("ftpfs module unloaded\n");
}

module_init(ftpfs_init);
module_exit(ftpfs_fini);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("vani");
