#include "ftpfs.h"
#include "ftp.h"
#include "inode.h"
#include "super.h"
#include "file.h"
#include <linux/mount.h>

const struct inode_operations ftp_fs_file_inode_operations = {
    .setattr = simple_setattr,
    .getattr = simple_getattr,
};

const struct inode_operations ftp_fs_dir_inode_operations = {
    .create = ftp_fs_create,
    .lookup = ftp_fs_lookup,
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

struct dentry* ftp_fs_lookup(struct inode* inode, struct dentry* dentry, unsigned int flags) {
	struct inode* target = NULL;

	pr_debug("process lookup %s\n", dentry->d_name.name);
	if (dentry->d_name.len > NAME_MAX)
		return ERR_PTR(-ENAMETOOLONG);
	if (!dentry->d_sb->s_d_op)
		d_set_d_op(dentry, &simple_dentry_operations);

	struct dentry *d = list_entry(inode->i_dentry.first, struct dentry, d_alias);
	char *filename = dentry->d_name.name;
	char *filebuf = (char*) kmalloc(MAX_PATH_LEN, GFP_KERNEL);
	if (filebuf == NULL) {
		pr_debug("allocate filebuf failed\n");
		goto out;
	}
	char *file_path = dentry_path_raw(d, filebuf, MAX_PATH_LEN);
	if (file_path == NULL) {
		pr_debug("calculate file path failed\n");
		goto error;
	}
	pr_debug("@lookup full path name %s\n", filename);

	int result = -1;
	unsigned long file_num;
	struct ftp_file_info *files;

	if ((result = ftp_read_dir((struct ftp_info*) inode->i_sb->s_fs_info, file_path, &file_num, &files)) == 0) {
		pr_debug("got %lu file\n", file_num);
		int i;
		for (i = 2; i < file_num; i++) if (strcmp(filename, files[i].name) == 0) {
			pr_debug("got this file\n");
			if ((target = ftp_fs_get_inode(inode->i_sb, inode, files[i].mode, 0)) == NULL) {
				pr_debug("can not allocate a inode\n");
				goto error;
			}
			/* missing m_time (need format converting) */
			target->i_size = files[i].size;

			pr_debug("new inode done\n");
			break;
		}
	}

error:
	if (filebuf) kfree(filebuf);
	pr_debug("freed filebuf\n");
out:
	d_add(dentry, target);
	pr_debug("add dentry\n");
	return NULL;
}

