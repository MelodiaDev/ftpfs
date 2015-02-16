#include "sock.h"
#include "ftpfs.h"

#include <asm/uaccess.h>
#include <linux/net.h>
#include <linux/slab.h>
#include <linux/in.h>

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
			return ret < 0 ? -1 : 0;
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

static unsigned short _htons(unsigned short port) {
    unsigned short s0 = port & (unsigned short)0x00ff;
    unsigned short s1 = port & (unsigned short)0xff00; 
    pr_debug("got the port %lu\n", (s0 << 8) + s1);
    return (s0 << 8) + s1;
}

static void _inet_aton(const char* ip, unsigned int *res) {
    unsigned int s[4];
    int i;
    sscanf(ip, "%lu.%lu.%lu.%lu", s, s + 1, s + 2, s + 3);
	*res = 0;
    for (i = 0; i < 4; i++) *res |= s[i] << (8 * i);
    pr_debug("%lu.%lu.%lu.%lu\n", s[0], s[1], s[2], s[3]);
    pr_debug("got the ip %lu\n", *res);
}

struct sockaddr_in* cons_addr(const char* ip) {
    struct sockaddr_in *addr = kmalloc(sizeof(struct sockaddr_in), GFP_KERNEL);
    if (addr) {
        addr->sin_family = AF_INET;
        addr->sin_port = _htons(FTP_PORT);
        _inet_aton(ip, &addr->sin_addr.s_addr);
    }
    return addr;
}
