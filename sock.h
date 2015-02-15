#ifndef _SOCK_H
#define _SOCK_H
#include <linux/net.h>

int sock_send(struct socket *sock, const void *buf, int len);
int sock_recv(struct socket *sock, void *buf, int size);
int sock_readline(struct socket *sock, char **buf);

struct sockaddr_in* cons_addr(const char*);

#endif
