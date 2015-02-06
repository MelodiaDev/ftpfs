#include "ftpfs.h"
#include "file.h"

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
