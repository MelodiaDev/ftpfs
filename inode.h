#ifndef _INODE_H
#define _INODE_H
extern const struct inode_operations ftp_fs_inode_operations;

struct inode* ftp_fs_get_inode(struct super_block *sb, const struct inode* dir, umode_t mode, dev_t dev);
#endif
