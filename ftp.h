#ifndef _FTP_H
#define _FTP_H
#include <linux/net.h>
#include <linux/in.h>
#include <linux/semaphore.h>

struct ftp_conn_info {
	struct socket *control_sock, *data_sock;
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

int ftp_info_init(struct ftp_info **info, struct sockaddr_in addr, const char *user, const char *pass, int max_sock);
void ftp_info_destroy(struct ftp_info *info);

#endif
