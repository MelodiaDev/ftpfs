#include "file.h"

const struct address_space_operations ftp_fs_aops = {
    .readpage = simple_readpage,
    .write_begin = simple_write_begin,
    .write_end = simple_write_end,
    .set_page_dirty = __set_page_dirty_no_writeback,
};

