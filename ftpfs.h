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
#include <asm/uaccess.h>

#define FTP_FS_DEFAULT_MODE 0755
#define FTP_FS_MAGIC 0x19950522

#endif
