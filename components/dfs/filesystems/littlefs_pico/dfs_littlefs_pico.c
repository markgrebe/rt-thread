/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2013-04-15     Bernard      the first version
 * 2013-05-05     Bernard      remove CRC for ramfs persistence
 * 2013-05-22     Bernard      fix the no entry issue.
 */

#include <rtthread.h>
#include <dfs.h>
#include <dfs_fs.h>
#include <dfs_file.h>
#include "pico_hal.h"
#include "dfs_littlefs_pico.h"

static int dfs_to_littlefs_flags(int flags);
static int lfs_error_to_dfs_error(int err);

int dfs_littlefs_mount(struct dfs_filesystem *fs,
                       unsigned long          rwflag,
                       const void            *data)
{
    struct dfs_littlefs *littlefs;

    if (pico_mount(false) == 0)
    {
        return RT_EOK;
    }
    else
        return -EIO;
}

int dfs_littlefs_unmount(struct dfs_filesystem *fs)
{
    pico_unmount();
    fs->data = NULL;

    return RT_EOK;
}

int dfs_littlefs_mkfs(rt_device_t dev_id)
{
    int status;

    status = pico_format();
    return lfs_error_to_dfs_error(status);
}

int dfs_littlefs_statfs(struct dfs_filesystem *fs, struct statfs *buf)
{
    struct pico_fsstat_t pico_stat;

    pico_fsstat(&pico_stat);

    RT_ASSERT(buf != NULL);

    buf->f_bsize  = pico_stat.block_size;
    buf->f_blocks = pico_stat.block_count;
    buf->f_bfree  = pico_stat.block_count - pico_stat.blocks_used;

    return RT_EOK;
}

int dfs_littlefs_ioctl(struct dfs_fd *fd, int cmd, void *args)
{
    return -EIO;
}

int dfs_littlefs_read(struct dfs_fd *fd, void *buf, size_t count)
{
    int count_read;

    count_read = pico_read((int) fd->data, buf, count);
    fd->pos = pico_tell((int) fd->data);
    return count_read;
}

int dfs_littlefs_write(struct dfs_fd *fd, const void *buf, size_t count)
{
    int count_read;

    count_read = pico_write((int) fd->data, buf, count);
    fd->pos = pico_tell((int) fd->data);
    return count_read;
}

int dfs_littlefs_flush(struct dfs_fd *fd)
{
    int result; 
    
    result = pico_fflush((int) fd->data);
    return lfs_error_to_dfs_error(result);
}

int dfs_littlefs_lseek(struct dfs_fd *file, off_t offset)
{
    int pos = pico_lseek((int) file->data, offset, LFS_SEEK_SET);
    if (pos < 0)
        return lfs_error_to_dfs_error(pos);
    
    file->pos = pos;
    return pos;
}

int dfs_littlefs_close(struct dfs_fd *file)
{
    int status;

    if (file->flags & O_DIRECTORY)
    {
        status = pico_dir_close((int) file->data);
    }
    else
    {
        status = pico_close((int) file->data);
    }
    file->data = NULL;

    return lfs_error_to_dfs_error(status);
}

static int dfs_to_littlefs_flags(int flags)
{
    int littlefs_flags = LFS_O_RDONLY;

    // open flags
    if (flags & O_RDWR)
        littlefs_flags |= LFS_O_RDONLY | LFS_O_WRONLY;
    if (flags & O_RDONLY)
        littlefs_flags |= LFS_O_RDONLY;
    if (flags & O_WRONLY)
        littlefs_flags |= LFS_O_WRONLY;
    if (flags & O_CREAT)
        littlefs_flags |= LFS_O_CREAT;
    if (flags & O_EXCL)
        littlefs_flags |= LFS_O_EXCL;
    if (flags & O_TRUNC)
        littlefs_flags |= LFS_O_TRUNC;
    if (flags & O_APPEND)
        littlefs_flags |= LFS_O_APPEND;
    return littlefs_flags;
}

// Possible error codes, these are negative to allow
// valid positive return values
static int lfs_error_to_dfs_error(int err)
{
    switch(err)
    {
        case LFS_ERR_IO:
            return -EIO;
        case LFS_ERR_CORRUPT:
            return -EIO;
        case LFS_ERR_NOENT:
            return -ENOENT;
        case LFS_ERR_EXIST:
            return -EEXIST;
        case LFS_ERR_NOTDIR:
            return -ENOTDIR;
        case LFS_ERR_ISDIR:
            return -EISDIR;
        case LFS_ERR_NOTEMPTY:
            return -ENOTEMPTY;
        case LFS_ERR_BADF:
            return -EBADF;
        case LFS_ERR_FBIG:
            return -EFBIG;
        case LFS_ERR_INVAL:
            return -EINVAL;
        case LFS_ERR_NOSPC:
            return -ENOSPC;
        case LFS_ERR_NOMEM:
            return -ENOMEM;
        case LFS_ERR_NOATTR:
            return -EIO;
        case LFS_ERR_NAMETOOLONG:
            return -ENAMETOOLONG;
        default:
            return -EIO;        
    }
}

int dfs_littlefs_open(struct dfs_fd *file)
{
    int littlefs_flags = dfs_to_littlefs_flags(file->flags);
    int fd;
    int status;

    if (file->flags & O_DIRECTORY)
    {
        if (file->flags & O_CREAT)
        {
            status = pico_mkdir(file->path);
            return lfs_error_to_dfs_error(status);
        }

        /* open directory */
        fd = pico_dir_open(file->path);
        file->pos = pico_tell(fd);
    }
    else
    {
        fd = pico_open(file->path, littlefs_flags);
        if (fd >= 0)
            file->pos = pico_tell(fd);
    }

    if (fd < 0)
        return lfs_error_to_dfs_error(fd);

    file->data = (void *) fd;
    file->size = pico_size(fd);

    return 0;
}

int dfs_littlefs_stat(struct dfs_filesystem *fs,
                      const char            *path,
                      struct stat           *st)
{
    struct lfs_info info;
    int status;

    status = pico_stat(path, &info);

    if (status < 0)
        return lfs_error_to_dfs_error(status);

    st->st_mode = S_IRUSR | S_IRGRP | S_IROTH |
                  S_IWUSR | S_IWGRP | S_IWOTH;
    if (info.type == LFS_TYPE_REG)
        st->st_mode |= S_IFREG;

    st->st_size = info.size;
    st->st_mtime = 0;

    return RT_EOK;
}

int dfs_littlefs_getdents(struct dfs_fd *fd,
                          struct dirent *dirp,
                          uint32_t    count)
{
    rt_size_t index;
    int status;
    int count_read = 0;
    struct lfs_info info;

    /* make integer count */
    count = (count / sizeof(struct dirent));
    if (count == 0)
        return -EINVAL;

    for (index = 0; index < count; index++)
    {
        status = pico_dir_read((int) fd->data, &info);
        if (status < 0 || info.type == 0)
            {
            //if (index == 0)
            //    return lfs_error_to_dfs_error(status);
            //else
                return count_read * sizeof(struct dirent);
            }

        if (info.type == LFS_TYPE_REG)
            dirp[index].d_type = DT_REG;
        else
            dirp[index].d_type = DT_DIR;

        dirp[index].d_namlen = (rt_uint8_t)rt_strlen(info.name);
        dirp[index].d_reclen = (rt_uint16_t)sizeof(struct dirent);
        rt_strncpy(dirp[index].d_name, info.name, LFS_NAME_MAX+1);

        count_read++;
        fd->pos += sizeof(struct dirent);
    }
    return count_read * sizeof(struct dirent);
}

int dfs_littlefs_unlink(struct dfs_filesystem *fs, const char *path)
{
    int status;

    status = pico_remove(path);
    
    if (status < 0)
        return lfs_error_to_dfs_error(status);

    else
        return RT_EOK;
}

int dfs_littlefs_rename(struct dfs_filesystem *fs,
                        const char            *oldpath,
                        const char            *newpath)
{
    int status;

    status = pico_rename(oldpath, newpath);
    
    if (status < 0)
        return lfs_error_to_dfs_error(status);
    else
        return RT_EOK;
}

static const struct dfs_file_ops _little_fops =
{
    dfs_littlefs_open,
    dfs_littlefs_close,
    dfs_littlefs_ioctl,
    dfs_littlefs_read,
    dfs_littlefs_write,
    dfs_littlefs_flush,
    dfs_littlefs_lseek,
    dfs_littlefs_getdents,
};

static const struct dfs_filesystem_ops _littlefs =
{
    "littlefs",
    DFS_FS_FLAG_DEFAULT,
    &_little_fops,

    dfs_littlefs_mount,
    dfs_littlefs_unmount,
    dfs_littlefs_mkfs,
    dfs_littlefs_statfs,

    dfs_littlefs_unlink,
    dfs_littlefs_stat,
    dfs_littlefs_rename,
};

static rt_err_t nop_init(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t nop_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t nop_close(rt_device_t dev)
{
    return RT_EOK;
}

static rt_size_t nop_read(rt_device_t dev,
                          rt_off_t    pos,
                          void       *buffer,
                          rt_size_t   size)
{
    return size;
}

static rt_size_t nop_write(rt_device_t dev,
                           rt_off_t    pos,
                           const void *buffer,
                           rt_size_t   size)
{
    return size;
}

static rt_err_t nop_control(rt_device_t dev, int cmd, void *args)
{
    return RT_EOK;
}

static struct rt_device littlefs_dev;
rt_err_t rt_littlefs_init(const char *name)
{
    rt_device_t dev;

    dev = &littlefs_dev;
    RT_ASSERT(dev != RT_NULL);

    /* set device class and generic device interface */
    dev->type        = RT_Device_Class_Block;
    dev->init        = nop_init;
    dev->open        = nop_open;
    dev->read        = nop_read;
    dev->write       = nop_write;
    dev->close       = nop_close;
    dev->control     = nop_control;

    dev->rx_indicate = RT_NULL;
    dev->tx_complete = RT_NULL;

    /* register to RT-Thread device system */
    return rt_device_register(dev, name, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_STANDALONE);
}

int dfs_littlefs_init(void)
{
    /* register ram file system */
    dfs_register(&_littlefs);

    /* register littlefs device */
    rt_littlefs_init("littlefs");

    /* Mount the filesystem */
    if (dfs_mount("littlefs", "/", "littlefs", 0, 0) == 0)
    {
        rt_kprintf("File System on root initialized!\n");
    }
    else
    {
        rt_kprintf("File System on root initialization failed!\n");
    }
    return 0;
}
INIT_COMPONENT_EXPORT(dfs_littlefs_init);
