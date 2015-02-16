#include "ftpfs.h"
#include "file.h"
#include "ftp.h"
#include "sock.h"

#include <linux/ctype.h>

const struct file_operations ftp_fs_file_operations = {
    .read = ftp_fs_read,
    .write = ftp_fs_write,
    .iterate = ftp_fs_iterate,
};

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

int ftp_fs_iterate(struct file* f, struct dir_context* ctx) {
    int result = -1;
    struct dentry *dentry = f->f_dentry;

    if (ctx->pos == 0) {
        pr_debug("dir_emit \".\" with i_node number %lu\n", dentry->d_inode->i_ino);
        if (dir_emit(ctx, ".", 1, dentry->d_inode->i_ino, DT_DIR) != 0) {
            pr_debug("dir_emit failed\n");
            goto error0;
        }
        ctx->pos = 1;
    }
    if (ctx->pos == 1) {
        pr_debug("dir_emit \"..\" with i_node number %lu\n", parent_ino(dentry));
        if (dir_emit(ctx, "..", 2, parent_ino(dentry), DT_DIR) != 0) {
            pr_debug("dir_emit failed\n");
            goto error0;
        }
        ctx->pos = 2;
    }

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

    unsigned long file_num;
    struct ftp_file_info *files;
    if ((result = ftp_read_dir(ftp_info, full_path, &file_num, &files)) == 0) {
        int i;
        for (i = 0; i < file_num; i++) {
            /* TODO, a fake inode number */
            pr_debug("dir_emt \"%s\" with i_node number %lu\n", files[i].name, 0lu);
            if (dir_emit(ctx, files[i].name, strlen(files[i].name), 0, (files[i].mode >> 12) & 25) != 0) {
                pr_debug("dir_emit failed\n");
                goto error3;
            }
            ctx->pos++;
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
    return result;
}
