#include "ftpfs.h"
#include "inode.h"
#include "super.h"
#include "file.h"

const struct inode_operations ftp_fs_file_inode_operations = {
    .setattr = simple_setattr,
    .getattr = simple_getattr,
};

const struct inode_operations ftp_fs_dir_inode_operations = {
    .create = ftp_fs_create,
    .lookup = simple_lookup,
	.mknod = ftp_fs_mknod,
	.link = simple_link,
	.unlink = simple_unlink,
};

struct inode* ftp_fs_get_inode(struct super_block *sb, const struct inode* dir, umode_t mode, dev_t dev) {
    struct inode* inode = new_inode(sb);
    if (inode) {
        inode->i_ino = get_next_ino();
        inode_init_owner(inode, dir, mode);
        inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;

        switch (mode & S_IFMT) {
            case S_IFREG:
                pr_debug("got a regular inode\n");
                inode->i_op = &ftp_fs_file_inode_operations;
                inode->i_fop = &ftp_fs_file_operations;
                break;
            case S_IFDIR:
                pr_debug("got a dir inode\n");
                inode->i_op = &ftp_fs_dir_inode_operations;
                inode->i_fop = &ftp_fs_dir_operations;
                break;
            default:
                pr_debug("got a special inode\n");
                init_special_inode(inode, mode, dev);
                break;
        }
    }
    return inode;
}

int ftp_fs_create(struct inode *dir, struct dentry *dentry, umode_t mode, bool excl) {
	return ftp_fs_mknod(dir, dentry, mode | S_IFREG, 0);
}

int ftp_fs_mknod(struct inode* dir, struct dentry* dentry, umode_t mode, dev_t dev) {
	struct inode* inode = ftp_fs_get_inode(dir->i_sb, dir, mode, dev);
	int error = -ENOSPC;

	if (inode) {
		d_instantiate(dentry, inode);
		dget(dentry);
		error = 0;
		dir->i_mtime = dir->i_ctime = CURRENT_TIME;
	}
	return error;
}

