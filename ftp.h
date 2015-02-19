/*
 * FTP side functions and data types.
 * On FTP side, max_sock sessions are maintained and upon receiving a
 * request, a session is chosen with a certain strategy.
 */
#ifndef _FTP_H
#define _FTP_H
#include <linux/net.h>
#include <linux/in.h>
#include <linux/semaphore.h>

/* Information about a FTP session. */
struct ftp_conn_info {
	/* Control socket (NULL if no session),
	 * and data socket (NULL if no data transfer) */
	struct socket *control_sock, *data_sock;
	/* Command for opening data transfer (NULL if no data transfer) */
	char *cmd;
	/* Offset number in data transfer */
	unsigned long offset;
	/* Mark if it is currently in use */
	int used;
};

/* Global information about the FTP side status. */
struct ftp_info {
	/* Semaphore indicating if there is a session available
	 * and mutex for locking session states. */
	struct semaphore sem, mutex;
	/* FTP server address */
	struct sockaddr_in addr;
	/* User name and password */
	char *user, *pass;
	/* Maximum number of sessions at a time */
	int max_sock;
	/* List of FTP sessions, an array of max_sock elements */
	struct ftp_conn_info *conn_list;
};

/* Information about a file item returned by ftp_read_file(), including
 * name, mode, number of links, file size, and last modified time. */
struct ftp_file_info {
	char *name;
	umode_t mode;
	nlink_t nlink;
	off_t size;
	time_t mtime;
};

/* Allocate space for global info and initialize it using provided arguments. */
int ftp_info_init(struct ftp_info **info, struct sockaddr_in addr,
		const char *user, const char *pass, int max_sock);
/* Deallocate global info */
void ftp_info_destroy(struct ftp_info *info);
/* Free the space allocated by ftp_read_dir(). */
void ftp_file_info_destroy(unsigned long len, struct ftp_file_info *files);
/* Read maximum <len> bytes from file <file> starting from offset <offset>. */
int ftp_read_file(struct ftp_info *info, const char *file,
		unsigned long offset, char *buf, unsigned long len);
/* Write <len> bytes to file <file> starting from offset <offset>. */
int ftp_write_file(struct ftp_info *info, const char *file,
		unsigned long offset, const char *buf, unsigned long len);
/* Close data transfer connections related to file <file>. */
void ftp_close_file(struct ftp_info *info, const char *file);
/* Retrieve info of all files contained in the directory <path>, store its
 * length in <len> and the starting pointer of the array in <files>,
 * whose space should later be freed by ftp_file_info_destroy(). */
int ftp_read_dir(struct ftp_info *info, const char *path,
		unsigned long *len, struct ftp_file_info **files);
/* Rename <oldpath> to <newpath>. */
int ftp_rename(struct ftp_info *info, const char *oldpath, const char *newpath);
/* Create a file at path <file>. */
int ftp_create_file(struct ftp_info *info, const char *file);
/* Remove a file at path <file>. */
int ftp_remove_file(struct ftp_info *info, const char *file);
/* Create a directory at path <path>. */
int ftp_create_dir(struct ftp_info *info, const char *path);
/* Remove a directory at path <path>. */
int ftp_remove_dir(struct ftp_info *info, const char *path);

#endif
