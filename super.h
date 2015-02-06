#ifndef _SUPER_H
#define _SUPER_H

extern const struct super_operations ftp_fs_ops;
extern struct file_system_type ftp_fs_type;
extern struct backing_dev_info ftp_fs_bdi;

struct dentry* ftp_fs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data);
int ftp_fs_fill_super(struct super_block *sb, void *data, int silent);
#endif
