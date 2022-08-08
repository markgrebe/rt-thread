/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author         Notes
 * 2021-01-28     flybreak       first version
 */

#include <rtthread.h>
#include <rtdevice.h>

#include <dfs_fs.h>
#include <dfs_littlefs_pico.h>

#define LED_PIN 25

int main(void)
{
    rt_kprintf("Hello, RT-Thread!\n");

    rt_pin_mode(LED_PIN, PIN_MODE_OUTPUT);

    while (1)
    {
        rt_pin_write(LED_PIN, 1);
        rt_thread_mdelay(1000);
        rt_pin_write(LED_PIN, 0);
        rt_thread_mdelay(1000);
    }
}

static int mark_times = 0;
FINSH_VAR_EXPORT(mark_times, finsh_type_int, dummy variable for finsh)

void marks_hello(int times)
{
    mark_times = times;
    rt_kprintf("Mark says hello %d times\n", times);
}

FINSH_FUNCTION_EXPORT(marks_hello , say hello from mark);
