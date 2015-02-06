#ifndef _FILE_H
#define _FILE_H
extern const struct address_space_operations ftp_fs_aops;
// TODO
extern const struct file_operations ftp_fs_file_operations;

ssize_t ftp_fs_read(struct file*, char __user*, size_t, loff_t*);
ssize_t ftp_fs_write(struct file*, const char __user*, size_t, loff_t*);
#endif
