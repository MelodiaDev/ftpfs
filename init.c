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

int __init ftpfs_init(void) {
    pr_debug("ftpfs module loaded\n");
    return 0;
}

void __exit ftpfs_fini(void) {
    pr_debug("ftpfs module unloaded\n");
}

struct ftp_fs_mount_opts {
    umode_t mode;
};

struct ftp_fs_info {
    struct ftp_fs_mount_opts mount_opts;
};

struct super_operations ftp_fs_ops {
    .statfs = simple_statfs,
    .drop_inode = generic_delete_inode,
    .show_options = generic_show_options,
};

static int ftp_fs_parse_options(char *data, struct ftp_fs_mount_opts *opts) {
    substring_t args[MAX_OPT_ARGS];
    int option;
    int token;
    char *p;

    opts->mode = RAMFS_DEFAULT_MODE;

    while ((p = strsep(&data, ",")) != NULL) {
        if (!*p) continue;

        token = match_token(p, tokens, args);
        switch (token) {
            case Opt_mode:
                if (match_octal(&args[0], &option))
                    return -EINVAL;
                opts->mode = option & S_IALLUGO;
                break;
        }
    }

    return 0;
}

int ftp_fs_fill_super(struct super_block *sb, void *data, int silent) {
    struct ftp_fs_info *fsi;
    struct inode* inode;
    int err;

    save_mount_options(sb, data);

    fsi = kzalloc(sizeof(ftp_fs_info), GFP_KERNEL);
    sb->s_fs_info = fsi;
    if (!fsi) return -ENOMEN;

    err = ftp_fs_parse_options(data, &fsi->mount_opts);
    if (err) return err;

    sb->s_maxbytes = MAX_LFS_FILESIZE;
    sb->s_blocksize = PAGE_CACHE_SIZE;
    sb->s_blocksize_bits = PAGE_CACHE_SHIFT;
    sb->s_magic = FTP_FS_MAGIC;
    sb->s_op = &ftp_fs_ops;
    sb->s_time_gran = 1;

    inode = ftp_fs_get_inode(sb, NULL, S_IFDIR | fsi->mount_opts.mode, 0);
    sb->s_root = d_make_root(inode);
    if (!sb->s_root) return -ENOMEN;

    return 0;
}

struct dentry* ftp_fs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data) {
    return mount_nodev(fs_type, flags, data, ftp_fs_fill_super);
}

static struct file_system_type ftp_fs_type {
    .name = "ftpfs",
    .mount = ftp_fs_mount,
    .kill_sb = ftp_fs_kill_sb,
    .fs_flags = FS_USERNS_MOUNT,
};


module_init(ftpfs_init);
module_exit(ftpfs_fini);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("vani");
