#ifndef _FTP_H
#define _FTP_H
#include <linux/net.h>
#include <linux/in.h>
#include <linux/semaphore.h>

struct ftp_conn_info {
	struct socket *control_sock, *data_sock;
	char *cmd;
	unsigned long offset;
	int used;
};

struct ftp_info {
	struct semaphore sem, mutex;
	struct sockaddr_in addr;
	char *user, *pass;
	int max_sock;
	struct ftp_conn_info *conn_list;
};

struct ftp_file_info {
	char *name;
	umode_t mode;
	nlink_t nlink;
	off_t size;
	time_t mtime;
};

int ftp_info_init(struct ftp_info **info, struct sockaddr_in addr, const char *user, const char *pass, int max_sock);
void ftp_info_destroy(struct ftp_info *info);
void ftp_file_info_destroy(unsigned long len, struct ftp_file_info *files);
int ftp_read_file(struct ftp_info *info, const char *file, unsigned long offset, char *buf, unsigned long len);
int ftp_write_file(struct ftp_info *info, const char *file, unsigned long offset, const char *buf, unsigned long len);
int ftp_read_dir(struct ftp_info *info, const char *path, unsigned long *len, struct ftp_file_info **files);
int ftp_rename(struct ftp_info *info, const char *oldpath, const char *newpath);
int ftp_create_file(struct ftp_info *info, const char *file);
int ftp_remove_file(struct ftp_info *info, const char *file);
int ftp_create_dir(struct ftp_info *info, const char *path);
int ftp_remove_dir(struct ftp_info *info, const char *path);

#endif
