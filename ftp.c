#include "ftp.h"
#include "sock.h"
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/time.h>

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

void ftp_file_info_destroy(unsigned long len, struct ftp_file_info *files) {
	unsigned long i = 0;
	for (; i < len; i++)
		kfree(files[i].name);
	kfree(files);
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
		if (ret < 0) {
			kfree(buf);
			return ret;
		}
		sent += ret;
	}
	return 0;
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
			if (ret >= 4 && buf[0] == buf2[0] && buf[1] == buf2[1] && buf[2] == buf2[2] && buf2[3] == ' ') {
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

static int ftp_connect(struct ftp_info *info, struct ftp_conn_info *conn) {
	int ret, bufsize, tmp;
	char *buf;
	bufsize = strlen(info->user) + 6;
	tmp = strlen(info->pass) + 6;
	if (tmp > bufsize)
		bufsize = tmp;
	if (sock_create(AF_INET, SOCK_STREAM, 0, &conn->control_sock) < 0)
		goto error0;
	if (conn->control_sock->ops->connect(conn->control_sock, (struct sockaddr*)&info->addr, sizeof(struct sockaddr_in), 0) < 0)
		goto error1;
	if (ftp_recv(conn, NULL) != 220)
		goto error1;
	buf = (char*)kmalloc(bufsize, GFP_KERNEL);
	if (buf == NULL)
		goto error1;
	sprintf(buf, "USER %s", info->user);
	if (ftp_send(conn, buf) < 0 || ((ret = ftp_recv(conn, NULL)) != 230 && ret != 331))
		goto error2;
	if (ret == 331) {
		sprintf(buf, "PASS %s", info->pass);
		if (ftp_send(conn, buf) < 0 || ftp_recv(conn, NULL) != 230)
			goto error2;
	}
	kfree(buf);
	if (ftp_send(conn, "TYPE I") < 0 || ftp_recv(conn, NULL) != 200)
		goto error1;
	return 0;
error2:
	kfree(buf);
error1:
	sock_release(conn->control_sock);
error0:
	return -1;
}

static int ftp_open_pasv(struct ftp_conn_info *conn) {
	char *resp, *ptr;
	struct sockaddr_in data_addr;
	int seg[6], i, ret;
	if (ftp_send(conn, "PASV") < 0)
		return -1;
	if ((ret = ftp_recv(conn, &resp)) != 227) {
		if (ret >= 0)
			kfree(resp);
		return -1;
	}
	ptr = resp + 4;
	for (; *ptr != 0 && (*ptr < '0' || *ptr > '9'); ptr++);
	if (*ptr == 0 || sscanf(ptr, "%d,%d,%d,%d,%d,%d", &seg[0], &seg[1], &seg[2], &seg[3], &seg[4], &seg[5]) < 6) {
		kfree(resp);
		return -1;
	}
	kfree(resp);
	for (i = 0; i < 6; i++)
		if (seg[i] < 0 || seg[i] >= 256)
			return -1;
	for (i = 0; i < 4; i++)
		((unsigned char*)&data_addr.sin_addr)[i] = seg[i];
	data_addr.sin_port = htons((seg[4] << 8) + seg[5]);
	if (sock_create(AF_INET, SOCK_STREAM, 0, &conn->data_sock) < 0)
		return -1;
	if (conn->data_sock->ops->connect(conn->data_sock, (struct sockaddr*)&data_addr, sizeof(struct sockaddr_in), 0) < 0) {
		sock_release(conn->data_sock);
		return -1;
	}
	return 0;
}

static void ftp_destroy_conn(struct ftp_conn_info *conn) {
	if (conn->data_sock != NULL) {
		sock_release(conn->data_sock);
		kfree(conn->cmd);
	}
	if (conn->control_sock != NULL)
		sock_release(conn->control_sock);
	conn->data_sock = conn->control_sock = NULL;
}

static void ftp_find_conn(struct ftp_info *info, struct ftp_conn_info **conn) {
	int i;
	for (i = 0; i < info->max_sock; i++)
		if (info->conn_list[i].used == 0 && info->conn_list[i].data_sock == NULL) {
			info->conn_list[i].used = 1;
			*conn = &info->conn_list[i];
			return;
		}
	for (i = 0; i < info->max_sock; i++)
		if (info->conn_list[i].used == 0) {
			info->conn_list[i].used = 1;
			*conn = &info->conn_list[i];
			ftp_destroy_conn(*conn);
			return;
		}
}

static int ftp_request_conn(struct ftp_info *info, struct ftp_conn_info **conn) {
	struct ftp_conn_info *tmp_conn;
	down(&info->sem);
	down(&info->mutex);
	ftp_find_conn(info, &tmp_conn);
	up(&info->mutex);
	if (tmp_conn->control_sock == NULL && ftp_connect(info, tmp_conn) < 0) {
		up(&info->sem);
		return -1;
	}
	*conn = tmp_conn;
	return 0;
}

static int ftp_request_conn_open_pasv(struct ftp_info *info, struct ftp_conn_info **conn, const char *cmd, const unsigned long offset) {
	struct ftp_conn_info *tmp_conn;
	int i;
	char *tmp_cmd, buf[256];
	down(&info->sem);
	down(&info->mutex);
	for (i = 0; i < info->max_sock; i++)
		if (info->conn_list[i].used == 0 && info->conn_list[i].data_sock != NULL
				&& strcmp(info->conn_list[i].cmd, cmd) == 0 && info->conn_list[i].offset == offset) {
			info->conn_list[i].used = 1;
			tmp_conn = &info->conn_list[i];
			up(&info->mutex);
			return 0;
		}
	ftp_find_conn(info, &tmp_conn);
	up(&info->mutex);
	if (tmp_conn->control_sock == NULL && ftp_connect(info, tmp_conn) < 0)
		goto error0;
	if (tmp_conn->data_sock == NULL && ftp_open_pasv(tmp_conn) < 0)
		goto error1;
	if (offset) {
		sprintf(buf, "REST %ld", offset);
		if (ftp_send(tmp_conn, buf) < 0 || ftp_recv(tmp_conn, NULL) != 350)
			goto error2;
	}
	if (ftp_send(tmp_conn, cmd) < 0 || ftp_recv(tmp_conn, NULL) != 150)
		goto error2;
	tmp_cmd = (char*)kmalloc(strlen(cmd) + 1, GFP_KERNEL);
	if (tmp_cmd == NULL)
		goto error2;
	strcpy(tmp_cmd, cmd);
	tmp_conn->cmd = tmp_cmd;
	tmp_conn->offset = offset;
	*conn = tmp_conn;
	return 0;
error2:
	sock_release(tmp_conn->data_sock);
error1:
	sock_release(tmp_conn->control_sock);
error0:
	up(&info->sem);
	return -1;
}

static void ftp_release_conn(struct ftp_info *info, struct ftp_conn_info *conn) {
	down(&info->mutex);
	conn->used = 0;
	up(&info->mutex);
	up(&info->sem);
}

static void ftp_release_conn_destroy(struct ftp_info *info, struct ftp_conn_info *conn) {
	ftp_destroy_conn(conn);
	ftp_release_conn(info, conn);
}

int ftp_read_file(struct ftp_info *info, const char *file, unsigned long offset, char *buf, unsigned long len) {
	struct ftp_conn_info *conn;
	char *cmd = (char*)kmalloc(strlen(file) + 8, GFP_KERNEL);
	int ret;
	if (cmd == NULL)
		return -1;
	sprintf(cmd, "STOR ./%s", file);
	if (ftp_request_conn_open_pasv(info, &conn, cmd, offset) < 0)
		return -1;
	ret = sock_send(conn->data_sock, buf, len);
	if (ret < 0) {
		ftp_release_conn_destroy(info, conn);
		return -1;
	}
	conn->offset += ret;
	ftp_release_conn(info, conn);
	return ret;
}

int ftp_write_file(struct ftp_info *info, const char *file, unsigned long offset, char *buf, unsigned long len) {
	struct ftp_conn_info *conn;
	char *cmd = (char*)kmalloc(strlen(file) + 8, GFP_KERNEL);
	int ret;
	if (cmd == NULL)
		return -1;
	sprintf(cmd, "RETR ./%s", file);
	if (ftp_request_conn_open_pasv(info, &conn, cmd, offset) < 0)
		return -1;
	ret = sock_recv(conn->data_sock, buf, len);
	if (ret < 0) {
		ftp_release_conn_destroy(info, conn);
		return -1;
	}
	conn->offset += ret;
	ftp_release_conn(info, conn);
	return ret;
}

int ftp_read_dir(struct ftp_info *info, const char *path, unsigned long *len, struct ftp_file_info **files) {
	static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	struct ftp_conn_info *conn;
	struct ftp_file_info *tmp_files;
	int ret, i, current_year, year, month, day, hour, min;
	unsigned long tmp_len, buf_len, tmp;
	struct timeval time;
	struct tm tm;
	char *cmd = (char*)kmalloc(strlen(path) + 12, GFP_KERNEL), *line, *ptr, *next;
	if (cmd == NULL)
		return -1;
	sprintf(cmd, "LIST -al ./%s", path);
	if (ftp_request_conn_open_pasv(info, &conn, cmd, 0) < 0)
		return -1;
	do_gettimeofday(&time);
	time_to_tm(time.tv_sec, 0, &tm);
	current_year = tm.tm_year;

	tmp_len = 0;
	buf_len = 16;
	tmp_files = (struct ftp_file_info*)kmalloc(16 * sizeof(struct ftp_file_info), GFP_KERNEL);
	if (tmp_files == NULL)
		goto error0;
	while ((ret = sock_readline(conn->data_sock, &line)) > 0) {
		if (tmp_len == buf_len) {
			struct ftp_file_info *tmp_files2 = (struct ftp_file_info*)kmalloc(2 * buf_len * sizeof(struct ftp_file_info), GFP_KERNEL);
			if (tmp_files2 == NULL)
				goto error1;
			memcpy(tmp_files2, tmp_files, buf_len * sizeof(struct ftp_file_info));
			kfree(tmp_files);
			tmp_files = tmp_files2;
		}

		ptr = line;
		for (i = 0; i < 8; i++) {
			for (; *ptr != 0 && *ptr == ' '; ptr++);
			if (*ptr == 0)
				goto error2;
			next = ptr;
			for (; *next != 0 && *next != ' '; next++);
			*next = 0;
			switch (i) {
				case 0:
					if (next - ptr != 10)
						goto error2;
					if (ptr[0] == 'd') tmp_files[tmp_len].mode |= S_IFDIR;
					else if (ptr[0] == 'l') tmp_files[tmp_len].mode |= S_IFLNK;
					else tmp_files[tmp_len].mode |= S_IFREG;
					if (ptr[1] == 'r') tmp_files[tmp_len].mode |= S_IRUSR;
					if (ptr[2] == 'w') tmp_files[tmp_len].mode |= S_IWUSR;
					if (ptr[3] == 'x') tmp_files[tmp_len].mode |= S_IXUSR;
					if (ptr[4] == 'r') tmp_files[tmp_len].mode |= S_IRGRP;
					if (ptr[5] == 'w') tmp_files[tmp_len].mode |= S_IWGRP;
					if (ptr[6] == 'x') tmp_files[tmp_len].mode |= S_IXGRP;
					if (ptr[7] == 'r') tmp_files[tmp_len].mode |= S_IROTH;
					if (ptr[8] == 'w') tmp_files[tmp_len].mode |= S_IWOTH;
					if (ptr[9] == 'x') tmp_files[tmp_len].mode |= S_IXOTH;
					break;
				case 1:
					if (sscanf(ptr, "%lu", &tmp) < 1)
						goto error2;
					tmp_files[tmp_len].nlink = tmp;
					break;
				case 2: case 3:
					break;
				case 4:
					if (sscanf(ptr, "%lu", &tmp) < 1)
						goto error2;
					tmp_files[tmp_len].size = tmp;
					break;
				case 5:
					if (next - ptr != 3)
						goto error2;
					month = 0;
					for (; month < 12; month++)
						if (strcmp(months[month], ptr) == 0)
							break;
					if (month == 12)
						goto error2;
					month++;
					break;
				case 6:
					if (sscanf(ptr, "%d", &day) < 1)
						goto error2;
					break;
				case 7:
					if (sscanf(ptr, "%d:%d", &hour, &min) == 2)
						year = current_year;
					else if (sscanf(ptr, "%d", &year) == 1)
						hour = min = 0;
					else
						goto error2;
					tmp_files[tmp_len].mtime = mktime(year, month, day, hour, min, 0);
					break;
			}
			*next = ' ';
			ptr = next;
		}

		//TODO: link
		for (; *ptr != 0 && *ptr == ' '; ptr++);
		if (*ptr == 0)
			goto error2;
		tmp_files[tmp_len].name = kmalloc(strlen(ptr) + 1, GFP_KERNEL);
		if (tmp_files[tmp_len].name == NULL)
			goto error2;
		strcpy(tmp_files[tmp_len].name, ptr);
		tmp_len++;
		kfree(line);
	}
	if (ret < 0)
		goto error1;
	sock_release(conn->data_sock);
	conn->data_sock = NULL;
	kfree(conn->cmd);
	ftp_release_conn(info, conn);
	*files = tmp_files;
	*len = tmp_len;
	return ret;
error2:
	kfree(line);
error1:
	ftp_file_info_destroy(tmp_len, tmp_files);
error0:
	ftp_release_conn_destroy(info, conn);
	return -1;
}
