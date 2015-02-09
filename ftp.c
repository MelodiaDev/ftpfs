#include "ftp.h"
#include "sock.h"
#include <linux/slab.h>
#include <linux/ctype.h>

int ftp_info_init(struct ftp_info **info, struct sockaddr_in addr, const char *user, const char *pass, int max_sock) {
	*info = (struct ftp_info*)kmalloc(sizeof(struct ftp_info), GFP_KERNEL);
	if (*info == NULL)
		goto error0;
	(*info)->user = (char*)kmalloc(strlen(user) + 1, GFP_KERNEL);
	if ((*info)->user == NULL)
		goto error1;
	(*info)->pass = (char*)kmalloc(strlen(pass) + 1, GFP_KERNEL);
	if ((*info)->pass == NULL)
		goto error2;
	(*info)->conn_list = (struct ftp_conn_info*)kmalloc(sizeof(struct ftp_conn_info) * max_sock, GFP_KERNEL);
	if ((*info)->conn_list == NULL)
		goto error3;
	memcpy(&(*info)->addr, &addr, sizeof(struct sockaddr_in));
	strcpy((*info)->user, user);
	strcpy((*info)->pass, pass);
	(*info)->max_sock = max_sock;
	memset((*info)->conn_list, 0, sizeof(struct ftp_conn_info) * max_sock);
	sema_init(&(*info)->sem, max_sock);
	sema_init(&(*info)->mutex, 1);
	return 0;
error3:
	kfree((*info)->pass);
error2:
	kfree((*info)->user);
error1:
	kfree(*info);
error0:
	return -1;
}

void ftp_info_destroy(struct ftp_info *info) {
	kfree(info->user);
	kfree(info->pass);
	kfree(info->conn_list);
	kfree(info);
}

static void ftp_request_conn(struct ftp_info *info, struct ftp_conn_info **conn) {
	int i;
	down(&info->sem);
	down(&info->mutex);
	for (i = 0; i < info->max_sock; i++)
		if (info->conn_list[i].used == 0) {
			info->conn_list[i].used = 1;
			*conn = &info->conn_list[i];
			break;
		}
	up(&info->mutex);
}

static void ftp_release_conn(struct ftp_info *info, struct ftp_conn_info *conn) {
	down(&info->mutex);
	conn->used = 0;
	up(&info->mutex);
	up(&info->sem);
}

static int ftp_send(struct ftp_conn_info *conn, const char *cmd) {
	int len, sent = 0, ret;
	char *buf = (char*)kmalloc(strlen(cmd) + 3, GFP_KERNEL);
	if (buf == NULL)
		return -1;
	sprintf(buf, "%s\r\n", cmd);
	len = strlen(buf);
	while (sent < len) {
		ret = sock_send(conn->control_sock, buf + sent, len - sent);
		if (ret < 0)
			goto error;
		sent += ret;
	}
	return 0;
error:
	kfree(buf);
	return ret;
}

static int ftp_recv(struct ftp_conn_info *conn, char **resp) {
	char *buf, *buf2;
	int ret = sock_readline(conn->control_sock, &buf), code;
	if (ret < 0)
		return ret;
	if (ret < 6 || (buf[3] != ' ' && buf[3] != '-')
			|| !isdigit(buf[0]) || !isdigit(buf[1]) || !isdigit(buf[2])
			|| buf[0] == '0') {
		kfree(buf);
		return -1;
	}
	sscanf(buf, "%d", &code);
	if (buf[3] == '-') {
		while (1) {
			ret = sock_readline(conn->control_sock, &buf2);
			if (ret >= 4 && memcmp(buf, buf2, 4) == 0) {
				if (resp != NULL)
					*resp = buf;
				else
					kfree(buf);
				kfree(buf2);
				return code;
			}
			if (ret < 0) {
				kfree(buf);
				return ret;
			}
			kfree(buf2);
		}
	} else {
		if (resp != NULL)
			*resp = buf;
		else
			kfree(buf);
		return code;
	}
}
