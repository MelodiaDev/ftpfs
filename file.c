#include "ftpfs.h"
#include "file.h"
#include "ftp.h"
#include "inode.h"

#include <linux/ctype.h>
#include <linux/fs.h>
#include <linux/dcache.h>

const struct file_operations ftp_fs_file_operations = {
    .read = ftp_fs_read,
    .write = ftp_fs_write,
    .release = ftp_fs_close,
};

const struct file_operations ftp_fs_dir_operations = {
    .open = ftp_fs_dir_open,
    .release = dcache_dir_close,
    .read = generic_read_dir,
    .iterate = ftp_fs_iterate,
};

ssize_t ftp_fs_read(struct file* f, char __user *buf, size_t count, loff_t *offset) {
    ssize_t content_size = -1;
    struct dentry *dentry = f->f_dentry;

    /* allocate the buffer to store the full path */
    pr_debug("begin to read\n");
    char *path_buf = (char*) kmalloc(MAX_PATH_LEN, GFP_KERNEL);
    if (path_buf == NULL)
        goto error0;
    char *full_path = dentry_path_raw(dentry, path_buf, MAX_PATH_LEN);

    /* read the file */
    pr_debug("file name is: %s\n", full_path);
    pr_debug("try to connect ftp server\n");
    content_size = ftp_read_file((struct ftp_info*) f->f_inode->i_sb->s_fs_info, full_path, *offset, buf, count);

    pr_debug("recieved content size: %lu\n", content_size);
    if (content_size != -1) *offset += content_size;

    kfree(path_buf);
error0:
    return content_size;
}

ssize_t ftp_fs_write(struct file* f, const char __user *buf, size_t count, loff_t* offset) {
    ssize_t content_size = -1;
    struct dentry *dentry = f->f_dentry;

    /* allocate the buffer to store the full path */
    pr_debug("begin to write\n");
    char *path_buf = (char*) kmalloc(MAX_PATH_LEN, GFP_KERNEL);
    if (path_buf == NULL)
        goto error0;
    char *full_path = dentry_path_raw(dentry, path_buf, MAX_PATH_LEN);

    /* write the file */
    pr_debug("file name is: %s\n", full_path);
    pr_debug("try to connect ftp server\n");
    content_size = ftp_write_file((struct ftp_info*) f->f_inode->i_sb->s_fs_info, full_path, *offset, buf, count);

    pr_debug("recieved content size: %lu\n", content_size);
    if (content_size != -1) *offset += content_size;

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

    /* get the full path for the file */
    char *path_buf = (char*) kmalloc(MAX_PATH_LEN, GFP_KERNEL);
    if (path_buf == NULL)
        goto error0;
    char *full_path = dentry_path_raw(dentry, path_buf, MAX_PATH_LEN);

    pr_debug("file name is: %s\n", full_path);

    unsigned long file_num;
    struct ftp_file_info *files;
    pr_debug("try to connect ftp server\n");
    if ((result = ftp_read_dir((struct ftp_info*) f->f_inode->i_sb->s_fs_info, full_path, &file_num, &files)) == 0) {
        pr_debug("got %lu files under the dir\n", file_num);

        struct fake_dentry_list *fake_dentry_head = NULL, *fake_dentry_last = NULL;
        /* allocate fake inodes */
        int i;
        for (i = 2 /* omit the . and .. */ ; i < file_num; i++) {
            pr_debug("the fake dentry name is %s\n", files[i].name);

            struct dentry *fake_dentry;
            struct qstr q;
            q.name = files[i].name;
            q.len = strlen(files[i].name);
            q.hash = full_name_hash(q.name, q.len);
            
            /* if the fake dentry exists, just skip it */
            if ((fake_dentry = d_lookup(dentry, &q)) != NULL) {
                dput(fake_dentry);
                continue;
            }
            
            /* allocate a fake dentry corresponding a remote file */
            fake_dentry= d_alloc_name(dentry, files[i].name);

            d_set_d_op(dentry, &simple_dentry_operations);
            /* add the fake dentry into hash table */
            d_add(fake_dentry, NULL);

            if (fake_dentry == NULL) {
                pr_debug("can not allocate a fake dentry for ls\n");
                result = -1;
                goto out;
            }

            /* allocate a fake inode corresponding a remote file */
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

    kfree(path_buf);
error0:
    pr_debug("iterate result is %d\n", result);
    return result;
}

int ftp_fs_dir_open(struct inode* inode, struct file* file) {
    pr_debug("opened file\n");
    /* set the current dir at this dir */
    static struct qstr cursor_name = QSTR_INIT(".", 1);
    file->private_data = d_alloc(file->f_path.dentry, &cursor_name);
    return file->private_data ? 0 : -ENOMEM;
}

int ftp_fs_close(struct inode* inode, struct file* file) {
    char *path_buf = (char*) kmalloc(MAX_PATH_LEN, GFP_KERNEL);
    if (path_buf == NULL)
        return 0;
    /* get the full path */
    char *full_path = dentry_path_raw(file->f_dentry, path_buf, MAX_PATH_LEN);
    ftp_close_file((struct ftp_info*) inode->i_sb->s_fs_info, full_path);
    kfree(path_buf);
    return 0;
}

