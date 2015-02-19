#include "ftpfs.h"
#include "super.h"
#include "inode.h"
#include "sock.h"
#include "ftp.h"

const struct super_operations ftp_fs_ops = {
    .statfs = simple_statfs,
    .drop_inode = generic_delete_inode,
    .show_options = generic_show_options,
};

int ftp_fs_fill_super(struct super_block *sb, void *data, int silent) {
    struct inode* inode;

    pr_debug("begin ftp_fs_fill_super\n");

    /* set init infomation for the super block */
    sb->s_maxbytes = MAX_LFS_FILESIZE;
    sb->s_blocksize = PAGE_CACHE_SIZE;
    sb->s_blocksize_bits = PAGE_CACHE_SHIFT;
    sb->s_magic = FTP_FS_MAGIC;
    sb->s_op = &ftp_fs_ops;
    sb->s_time_gran = 1;

    /* initialize the gloabl ftp_info including the socket informations
     * and point the sb->s_fs_info to it */

    struct sockaddr_in *addr = cons_addr(FTP_IP);
    if (addr == NULL) return -1;
    struct ftp_info *ftp_info;
    if (ftp_info_init(&ftp_info, *addr, FTP_USERNAME, FTP_PASSWORD, MAX_SOCK) == -1) {
        kfree(addr);
        return -1;
    }

    sb->s_fs_info = ftp_info;

    /* get a inode ref for the super block */
    pr_debug("try to fetch a inode to store super block\n");
    inode = ftp_fs_get_inode(sb, NULL, S_IFDIR, 0);
    sb->s_root = d_make_root(inode);
    if (!sb->s_root) return -ENOMEM;

    return 0;
}

struct dentry* ftp_fs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data) {
    return mount_nodev(fs_type, flags, data, ftp_fs_fill_super);
}

void ftp_fs_umount(struct super_block *sb) {
    /* free the ftp_info struct */
    if (sb->s_fs_info) kfree(sb->s_fs_info);
    kill_litter_super(sb);
}

struct file_system_type ftp_fs_type = {
    .name = "ftpfs",
    .mount = ftp_fs_mount,
    .kill_sb = ftp_fs_umount,
};

