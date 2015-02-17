#include "ftpfs.h"
#include "file.h"
#include "ftp.h"
#include "sock.h"
#include "inode.h"

#include <linux/ctype.h>
#include <linux/fs.h>
#include <linux/dcache.h>

const struct file_operations ftp_fs_file_operations = {
    .read = ftp_fs_read,
    .write = ftp_fs_write,
};

const struct file_operations ftp_fs_dir_operations = {
    .open = ftp_fs_dir_open,
	.release = dcache_dir_close,
	.read = generic_read_dir,
    .iterate = ftp_fs_iterate,
};

const struct dentry_operations simple_dentry_operations = {
	.d_delete = always_delete_dentry,
};

int always_delete_dentry(const struct dentry *dentry) { return 1; }

ssize_t ftp_fs_read(struct file* f, char __user *buf, size_t count, loff_t *offset) {
    ssize_t content_size = -1;
    struct dentry *dentry = f->f_dentry;

    pr_debug("begin to read\n");
    char *path_buf = (char*) kmalloc(MAX_PATH_LEN, GFP_KERNEL);
    if (path_buf == NULL)
        goto error0;
    char *full_path = dentry_path_raw(dentry, path_buf, MAX_PATH_LEN);

    pr_debug("file name is: %s\n", full_path);
    struct sockaddr_in *addr = cons_addr(FTP_IP);
    if (addr == NULL)
        goto error1;
    struct ftp_info *ftp_info;
    if (ftp_info_init(&ftp_info, *addr, FTP_USERNAME, FTP_PASSWORD, MAX_SOCK) == -1) {
        goto error2;
    }

    pr_debug("try to connect ftp server\n");
    content_size = ftp_read_file(ftp_info, full_path, *offset, buf, count);

    pr_debug("recieved content size: %lu\n", content_size);
    if (content_size != -1) *offset += content_size;

    ftp_info_destroy(ftp_info);
error2:
    kfree(addr);
error1:
    kfree(path_buf);
error0:
    return content_size;
}

ssize_t ftp_fs_write(struct file* f, const char __user *buf, size_t count, loff_t* offset) {
    ssize_t content_size = -1;
    struct dentry *dentry = f->f_dentry;

    pr_debug("begin to write\n");
    char *path_buf = (char*) kmalloc(MAX_PATH_LEN, GFP_KERNEL);
    if (path_buf == NULL)
        goto error0;
    char *full_path = dentry_path_raw(dentry, path_buf, MAX_PATH_LEN);

    pr_debug("file name is: %s\n", full_path);
    struct sockaddr_in *addr = cons_addr(FTP_IP);
    if (addr == NULL)
        goto error1;
    struct ftp_info *ftp_info;
    if (ftp_info_init(&ftp_info, *addr, FTP_USERNAME, FTP_PASSWORD, MAX_SOCK) == -1) {
        goto error2;
    }

    pr_debug("try to connect ftp server\n");
    content_size = ftp_write_file(ftp_info, full_path, *offset, buf, count);

    pr_debug("recieved content size: %lu\n", content_size);
    if (content_size != -1) *offset += content_size;

    ftp_info_destroy(ftp_info);
error2:
    kfree(addr);
error1:
    kfree(path_buf);
error0:
    return content_size;
}

struct fake_dentry_list {
	struct dentry* dentry;
	struct fake_dentry_list* next;
};

inline unsigned char _dt_type(struct inode* inode) {
    return (inode->i_mode >> 12) & 15;
}

inline int simple_positive(struct dentry *dentry) {
    return dentry->d_inode && !d_unhashed(dentry);
}

int ftp_fs_iterate(struct file* f, struct dir_context* ctx) {
    pr_debug("begin to iterate\n");
    if (!dir_emit_dots(f, ctx)) {
        pr_debug("emit dot failed\n");
        return 0;
    }

    int result = -1;
    struct dentry *dentry = f->f_dentry;

    char *path_buf = (char*) kmalloc(MAX_PATH_LEN, GFP_KERNEL);
    if (path_buf == NULL)
        goto error0;
    char *full_path = dentry_path_raw(dentry, path_buf, MAX_PATH_LEN);

    pr_debug("file name is: %s\n", full_path);
    struct sockaddr_in *addr = cons_addr(FTP_IP);
    if (addr == NULL)
        goto error1;
    struct ftp_info *ftp_info;
    if (ftp_info_init(&ftp_info, *addr, FTP_USERNAME, FTP_PASSWORD, MAX_SOCK) == -1) {
        goto error2;
    }

    unsigned long file_num;
    struct ftp_file_info *files;
    pr_debug("try to connect ftp server\n");
    if ((result = ftp_read_dir(ftp_info, full_path, &file_num, &files)) == 0) {
        pr_debug("got %lu files under the dir\n", file_num);

		struct fake_dentry_list *fake_dentry_head = NULL, *fake_dentry_last = NULL;
		/* allocate fake inodes */
        int i;
		for (i = 2 /* omit the . and .. */ ; i < file_num; i++) {
			pr_debug("the fake dentry name is %s\n", files[i].name);

			struct dentry *fake_dentry = d_alloc_name(dentry, files[i].name);
			d_set_d_op(dentry, &simple_dentry_operations);
			d_add(fake_dentry, NULL);

			if (fake_dentry == NULL) {
				pr_debug("can not allocate a fake dentry for ls\n");
				result = -1;
				goto out;
			}

			if (ftp_fs_mknod(dentry->d_inode, fake_dentry, files[i].mode, 0) != 0) {
				pr_debug("can not allocate a inode for fake dentry\n");
				result = -1;
				goto out;
			}

			/* fill the information to inode (missing time format converting) */
			/* fake_dentry->d_inode->i_mtime = files[i].mtime; */
			fake_dentry->d_inode->i_size = files[i].size;

			/* update the list */
			struct fake_dentry_list* tmp = (struct fake_dentry_list*) kmalloc(sizeof(struct fake_dentry_list), GFP_KERNEL);
			if (tmp == NULL) {
				pr_debug("allocate fake_dentry_list node failed\n");
				result = -1;
				goto out;
			}
			tmp->next = NULL;
			tmp->dentry = fake_dentry;
			if (fake_dentry_last == NULL) {
				fake_dentry_last = fake_dentry_head = tmp;
			} else {
				fake_dentry_last->next = tmp;
				fake_dentry_last = tmp;
			}
		}

		dcache_readdir(f, ctx);
out:
		/* free the fake dentry */
		pr_debug("fake dentry lists: \n");
		struct fake_dentry_list *ptr;
		for (ptr = fake_dentry_head; ptr;) {
			pr_debug("    %s\n", ptr->dentry->d_name.name);
			simple_unlink(dentry->d_inode, ptr->dentry);
			struct fake_dentry_list *next = ptr->next;
			kfree(ptr);
			ptr = next;
		}

        ftp_file_info_destroy(file_num, files);
    }

error3:
    ftp_info_destroy(ftp_info);
error2:
    kfree(addr);
error1:
    kfree(path_buf);
error0:
	pr_debug("iterate result is %d\n", result);
    return result;
}

int ftp_fs_dir_open(struct inode* inode, struct file* file) {
	pr_debug("opened file\n");
	static struct qstr cursor_name = QSTR_INIT(".", 1);
	file->private_data = d_alloc(file->f_path.dentry, &cursor_name);
	return file->private_data ? 0 : -ENOMEM;
}

