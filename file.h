#ifndef _FILE_H
#define _FILE_H
// TODO
extern const struct file_operations ftp_fs_file_operations;
extern const struct file_operations ftp_fs_dir_operations;

ssize_t ftp_fs_read(struct file*, char __user*, size_t, loff_t*);
ssize_t ftp_fs_write(struct file*, const char __user*, size_t, loff_t*);
int ftp_fs_iterate(struct file* f, struct dir_context* ctx);
int ftp_fs_dir_open(struct inode* inode, struct file* file);
#endif
