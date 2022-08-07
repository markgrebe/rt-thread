/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2013-04-15     Bernard      the first version
 * 2013-05-05     Bernard      remove CRC for ramfs persistence
 */

#ifndef __DFS_LITTLEFS_H__
#define __DFS_LITTLEFS_H__

#include <rtthread.h>
#include <rtservice.h>

int dfs_littlefs_init(void);
rt_err_t rt_littlefs_init(const char *name);

#endif

