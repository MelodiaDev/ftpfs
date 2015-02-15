#include "ftpfs.h"
#include "inode.h"
#include "super.h"
#include "file.h"

const struct inode_operations ftp_fs_file_inode_operations = {
    .setattr = simple_setattr,
    .getattr = simple_getattr,
};


struct inode* ftp_fs_get_inode(struct super_block *sb, const struct inode* dir, umode_t mode, dev_t dev) {
    struct inode* inode = new_inode(sb);
    if (inode) {
        inode->i_ino = get_next_ino();
        inode_init_owner(inode, dir, mode);
        inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;

        switch (mode & S_IFMT) {
            case S_IFREG:
                inode->i_op = &ftp_fs_file_inode_operations;
                inode->i_fop = &ftp_fs_file_operations;
                break;
            case S_IFDIR:
                inode->i_op = &ftp_fs_file_inode_operations;
                inode->i_fop = &ftp_fs_file_operations;
                break;
            default:
                init_special_inode(inode, mode, dev);
                break;
        }
    }
    return inode;
}

