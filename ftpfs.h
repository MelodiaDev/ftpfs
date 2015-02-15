#ifndef _FTPFS_H
#define _FTPFS_H

#include <linux/mm.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/highmem.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/backing-dev.h>
#include <linux/ramfs.h>
#include <linux/sched.h>
#include <linux/parser.h>
#include <linux/magic.h>
#include <linux/slab.h>
#include <linux/compiler.h>
#include <linux/types.h>
#include <asm/uaccess.h>

#define FTP_FS_DEFAULT_MODE 0755
#define FTP_FS_MAGIC 0x19950522

#define MAX_PATH_LEN (50 * sizeof (char))
#define MAX_CONTENT_SIZE 52428800

#define FTP_IP "192.168.1.1"
#define FTP_USERNAME "vani"
#define FTP_PASSWORD "123456"

#define MAX_SOCK 5
#define FTP_PORT 20u

#endif
