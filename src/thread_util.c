/*
 * Copyright (c) 2022, Mark Grebe
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2006-03-28     Bernard      first version
 */

#include <rthw.h>
#include <rtthread.h>
#include <stddef.h>

#define TCS_STACK_SIZE  (8*1024)
#define TCS_PRIORITY    (24)
#define TCS_SLICE       (5)

static int thread_num = 0;

/**
 * @brief   This function creates and starts a thread with default
 *          stack size, priority, and time slice.  The name of the
 *          created thread is tx, where x is an increasing integer
 *          that starts at zero.
 *
 * @param   entry is the entry function of thread.
 *
 * @param   parameter is the parameter of thread entry function.
 *
 * @return  If the return value is a rt_thread structure pointer, the function is successfully executed.
 *          If the return value is RT_NULL, it means this operation failed.
*/
rt_thread_t tcs(void (*entry)(void *parameter),
                void  *parameter)
    {
    char namebuf[10];
    rt_thread_t tid;

    snprintf(namebuf,9,"t%d",thread_num++);
    tid = rt_thread_create(namebuf, entry, parameter, 
                           TCS_STACK_SIZE, TCS_PRIORITY, TCS_SLICE);
    if (tid != NULL)
        rt_thread_startup(tid);
    
    return(tid);
    }

RTM_EXPORT(tcs);
CS4000_FUNCTION_EXPORT(tcs, tcs, Create and Start a Thread);

typedef struct repeat_p
    {
    int  count;
    void (*entry)(void *parameter);
    void * parameter;
    } REPEAT_P;

static void repeat_entry(void *parameter)
    {
    REPEAT_P *r_parameter = (REPEAT_P *)parameter;
    int i;

    if (r_parameter->count == 0)
        {
        while(1)
            {
            (*(r_parameter->entry))(r_parameter->parameter);
            }
        }
    else
        {
        for (i=0; i<r_parameter->count; i++)
            {
            (*(r_parameter->entry))(r_parameter->parameter);
            }
        }
    }

/**
 * @brief   This function creates and starts a task that repeats
 *          a given function a given number of times.
 *
 * @param   count is the number of times to repeat the function.
 *          A value of 0 means an infinite number of times.
 *
 * @param   entry is the entry function of thread.
 *
 * @param   parameter is the parameter of thread entry function.
 *
 * @return  If the return value is a rt_thread structure pointer, the function is successfully executed.
 *          If the return value is RT_NULL, it means this operation failed.
*/
rt_thread_t repeat(int count,
                   void (*entry)(void *parameter),
                   void  *parameter)
    {
    REPEAT_P r_parameter;
    r_parameter.count = count;
    r_parameter.entry = entry;
    r_parameter.parameter = parameter;

    return tcs(repeat_entry, (void *) &r_parameter);
    }

RTM_EXPORT(repeat);
CS4000_FUNCTION_EXPORT(repeat, repeat, Create thread which calls function n times);

typedef struct period_p
    {
    int  secs;
    void (*entry)(void *parameter);
    void * parameter;
    } PERIOD_P;

static void period_entry(void *parameter)
    {
    PERIOD_P *p_parameter = (PERIOD_P *)parameter;
    int delay = 1000 * p_parameter->secs;
    void (*func)(void *) = p_parameter->entry;
    void * func_parameter = p_parameter->parameter;

    while(1)
        {
        (*func)(func_parameter);
        rt_thread_mdelay(delay);
        }
    }

/**
 * @brief   This function creates and starts a task that repeats
 *          a given function every @secs seconds..
 *
 * @param   secs is the number of seconds to delay bewteen each
 *          invocation of the function.
 *
 * @param   entry is the entry function of thread.
 *
 * @param   parameter is the parameter of thread entry function.
 *
 * @return  If the return value is a rt_thread structure pointer, the function is successfully executed.
 *          If the return value is RT_NULL, it means this operation failed.
*/
rt_thread_t period(int secs,
                   void (*entry)(void *parameter),
                   void  *parameter)
    {
    PERIOD_P p_parameter;
    p_parameter.secs = secs;
    p_parameter.entry = entry;
    p_parameter.parameter = parameter;

    return tcs(period_entry, (void *) &p_parameter);
    }

RTM_EXPORT(rt_period);
CS4000_FUNCTION_EXPORT(period, period, Create thread which calls function every n secs);

rt_err_t rt_thread_priority(rt_thread_t thread, int prior)
    {
    return(rt_thread_control(thread, RT_THREAD_CTRL_CHANGE_PRIORITY,
                             &prior));
    }

RTM_EXPORT(rt_thread_priority);
CS4000_FUNCTION_EXPORT(rt_thread_priority, t_priority, Set thread priority);
