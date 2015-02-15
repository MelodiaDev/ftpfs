#include "ftpfs.h"
#include "file.h"
#include "ftp.h"


const struct address_space_operations ftp_fs_aops = {
    .readpage = simple_readpage,
    .write_begin = simple_write_begin,
    .write_end = simple_write_end,
    .set_page_dirty = __set_page_dirty_no_writeback, // TODO there is a WARNING and I don't know why
};

const struct file_operations ftp_fs_file_operations = {
    .read = ftp_fs_read,
    .write = ftp_fs_write,
};

ssize_t ftp_fs_read(struct file* f, char __user *buf, size_t count, loff_t *offset) {
    ssize_t content_size = -1;
    struct dentry *dentry = f->f_dentry;

    char *buf = (char*) kmalloc(MAX_PATH_LEN, GFP_KERNEL);
    if (buf == NULL)
        goto error0;
    char *full_path = dentry_path_raw(dentry, buf, MAX_PATH_LEN);

    struct sockaddr_in *addr = cons_addr(FTP_IP);
    if (addr == NULL)
        goto error1;
    struct ftp_info *ftp_info;
    if (ftp_info_init(&ftp_info, addr, FTP_USERNAME, FTP_PASSWORD, MAX_SOCK) == -1) {
        goto error2;
    }

    ssize_t content_size =
        ftp_read_file(ftp_info, full_path, *offset, buf, count);

    if (content_size != -1) *offset += content_size;

    ftp_info_destroy(ftp_info);
error2:
    kfree(addr);
error1:
    kfree(buf);
error0:
    return content_size;
}

ssize_t ftp_fs_write(struct file* f, const char __user *buf, size_t count, loff_t* offset) {
    ssize_t content_size = -1;
    struct dentry *dentry = f->f_dentry;

    char *buf = (char*) kmalloc(MAX_PATH_LEN, GFP_KERNEL);
    if (buf == NULL)
        goto error0;
    char *full_path = dentry_path_raw(dentry, buf, MAX_PATH_LEN);

    struct sockaddr_in *addr = cons_addr(FTP_IP);
    if (addr == NULL)
        goto error1;
    struct ftp_info *ftp_info;
    if (ftp_info_init(&ftp_info, addr, FTP_USERNAME, FTP_PASSWORD, MAX_SOCK) == -1) {
        goto error2;
    }

    ssize_t content_size =
        ftp_write_file(ftp_info, full_path, *offset, buf, count);

    if (content_size != -1) *offset += content_size;

    ftp_info_destroy(ftp_info);
error2:
    kfree(addr);
error1:
    kfree(buf);
error0:
    return content_size;
}
