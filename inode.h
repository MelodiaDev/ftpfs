#ifndef _INODE_H
#define _INODE_H
// TODO
extern const struct inode_operations ftp_fs_file_inode_operations;
// TODO
extern const struct inode_operations ftp_fs_dir_inode_operations;

struct inode* ftp_fs_get_inode(struct super_block *sb, const struct inode* dir, umode_t mode, dev_t dev);

// inode operations
int ftp_fs_create(struct inode* inode, struct dentry* dentry, umode_t mode, bool flag);
struct dentry* ftp_fs_lookup(struct inode* inode, struct dentry* dentry, unsigned int flag);
int ftp_fs_mkdir(struct inode* inode, struct dentry* dentry, umode_t mode);
int ftp_fs_rmdir(struct inode* inode, struct dentry* dentry);

#endif
