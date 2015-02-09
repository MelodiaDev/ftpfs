#include <asm/uaccess.h>
#include <linux/net.h>
#include <linux/slab.h>

int sock_send(struct socket *sock, const void *buf, int len) {
	struct iovec iov;
	struct msghdr msg;
	int ret;
	mm_segment_t old_fs = get_fs();
	set_fs(get_ds());

	iov.iov_base = (void*)buf;
	iov.iov_len = len;
	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	ret = sock_sendmsg(sock, &msg, len);

	set_fs(old_fs);
	return ret;
}

int sock_recv(struct socket *sock, void *buf, int size) {
	struct iovec iov;
	struct msghdr msg;
	int ret;
	mm_segment_t old_fs = get_fs();
	set_fs(get_ds());

	iov.iov_base = buf;
	iov.iov_len = size;
	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	ret = sock_recvmsg(sock, &msg, size, 0);

	set_fs(old_fs);
	return ret;
}

int sock_readline(struct socket *sock, char **buf) {
	int size = 4096, read = 0, ret;
	char *tmp;
	*buf = kmalloc(size, GFP_KERNEL);
	if (*buf == NULL)
		return -1;

	while (1) {
		ret = sock_recv(sock, *buf + read, 1);
		if (ret <= 0) {
			kfree(*buf);
			return -1;
		}
		read++;
		if (*buf[read - 1] == '\n') {
			buf[read] = 0;
			return read;
		}
		if (read == size - 1) {
			tmp = kmalloc(size * 2, GFP_KERNEL);
			if (tmp == NULL) {
				kfree(*buf);
				//TODO: debug
				return -1;
			}
			memcpy(tmp, *buf, read);
			kfree(*buf);
			*buf = tmp;
			size *= 2;
		}
	}
}
