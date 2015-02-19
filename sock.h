/*
 * Socket functions in kernel space.
 */
#ifndef _SOCK_H
#define _SOCK_H
#include <linux/net.h>

/* Send a chunk of data, analogous to send() in user space.
 * Return value: same as sock_sendmsg(). */
int sock_send(struct socket *sock, const void *buf, int len);
/* Receive a chunk of data, analogous to recv() in user space.
 * Return value: same as sock_recvmsg(). */
int sock_recv(struct socket *sock, void *buf, int size);
/* Read a line of data and return the start pointer of the line in <buf>,
 * which should later be kfree()d. The line is padded with '\0'.
 * Return value: length of the line on success; otherwise 0 for an incomplete
 * line before closing connection or -1 for error, and <buf> is not set. */
int sock_readline(struct socket *sock, char **buf);

/* Convert the dot-represented IPv4 address to a sockaddr_in struct allocated,
 * which should later be kfree()d.
 * Return value: pointer to the address or NULL for error. */
struct sockaddr_in* cons_addr(const char*);

#endif
