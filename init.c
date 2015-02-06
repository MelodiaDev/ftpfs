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

#define FTP_FS_DEFAULT_MODE 0755
#define FTP_FS_MAGIC 0x19950522

static const struct super_operations ftp_fs_ops;
static const struct inode_operations ftp_fs_inode_operations;

struct inode* ftp_fs_get_inode(struct super_block *sb, const struct inode *dir, umode_t mode, dev_t dev) {
    return NULL;
}

static const struct super_operations ftp_fs_ops = {
    .statfs = simple_statfs,
    .drop_inode = generic_delete_inode,
    .show_options = generic_show_options,
};

static struct backing_dev_info ftp_fs_bdi = {
    .name = "ftpfs",
    .ra_pages = 0,
    .capabilities = BDI_CAP_NO_ACCT_AND_WRITEBACK |
                    BDI_CAP_MAP_DIRECT | BDI_CAP_MAP_COPY |
                    BDI_CAP_READ_MAP | BDI_CAP_WRITE_MAP | BDI_CAP_EXEC_MAP,
};

struct ftp_fs_mount_opts {
    umode_t mode;
};

struct ftp_fs_info {
    struct ftp_fs_mount_opts mount_opts;
};

enum {
    Opt_mode, Opt_err
};

static const match_table_t tokens = {
    {Opt_mode, "mode=%o"},
    {Opt_err, NULL}
};

static int ftp_fs_parse_options(char *data, struct ftp_fs_mount_opts *opts) {
    substring_t args[MAX_OPT_ARGS];
    int option;
    int token;
    char *p;

    opts->mode = FTP_FS_DEFAULT_MODE;

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

    fsi = kzalloc(sizeof(struct ftp_fs_info), GFP_KERNEL);
    sb->s_fs_info = fsi;
    if (!fsi) return -ENOMEM;

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
    if (!sb->s_root) return -ENOMEM;

    return 0;
}

static void ftp_fs_kill_sb(struct super_block *sb) {
    kfree(sb->s_fs_info);
    kill_litter_super(sb);
}

struct dentry* ftp_fs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data) {
    return mount_nodev(fs_type, flags, data, ftp_fs_fill_super);
}

static struct file_system_type ftp_fs_type = {
    .name = "ftpfs",
    .mount = ftp_fs_mount,
    .kill_sb = ftp_fs_kill_sb,
    .fs_flags = FS_USERNS_MOUNT,
};

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
