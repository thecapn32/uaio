// Copyright 2023 Vahid Mardani
/*
 * This file is part of uaio.
 *  uaio is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  uaio is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with uaio. If not, see <https://www.gnu.org/licenses/>.
 *
 *  Author: Vahid Mardani <vahid.mardani@gmail.com>
 */
#ifndef UAIO_H_
#define UAIO_H_


#include <stddef.h>
#include "esp_timer.h"


enum uaio_taskstatus {
    UAIO_IDLE = 1,
    UAIO_RUNNING = 2,
    UAIO_WAITING = 4,
    UAIO_TERMINATING = 8,
    UAIO_TERMINATED = 16,
};


struct uaio;
struct uaio_task;
typedef void (*uaio_invoker) (struct uaio_task *self);


struct uaio_basecall {
    struct uaio_basecall *parent;
    int line;
    uaio_invoker invoke;
};


struct uaio_task {
    struct uaio* uaio;
    struct uaio_basecall *current;
    enum uaio_taskstatus status;
    esp_timer_handle_t sleep;
    int eno;
};


/* Modules */
#ifdef CONFIG_UAIO_MODULES

struct uaio_module;
typedef int (*uaio_hook) (struct uaio *c, struct uaio_module *m);
typedef int (*uaio_tick) (struct uaio *c, struct uaio_module *m,
        unsigned int timeout_us);
struct uaio_module {
    uaio_hook loopstart;
    uaio_tick tick;
    uaio_hook loopend;
};


int
uaio_module_install(struct uaio *c, struct uaio_module *m);


int
uaio_module_uninstall(struct uaio *c, struct uaio_module *m);


/* fdmon module */
#ifdef CONFIG_UAIO_FDMON

struct uaio_fdmon;
typedef int (*uaio_filemonitor) (struct uaio_fdmon *iom,
        struct uaio_task *task, int fd, int events);
typedef int (*uaio_fileforget) (struct uaio_fdmon *iom, int fd);
struct uaio_fdmon {
    struct uaio_module;
    uaio_filemonitor monitor;
    uaio_fileforget forget;
};


#define UAIO_FILE_FORGET(fdmon, fd) (fdmon)->forget(fdmon, fd)
#define UAIO_FILE_AWAIT(fdmon, task, fd, events) \
    do { \
        (task)->current->line = __LINE__; \
        if ((fdmon)->monitor(fdmon, task, fd, events)) { \
            (task)->status = UAIO_TERMINATING; \
        } \
        else { \
            (task)->status = UAIO_WAITING; \
        } \
        return; \
        case __LINE__:; \
    } while (0)


/* IO helper macros */
#define UAIO_IN 0x1
#define UAIO_ERR 0x2
#define UAIO_OUT 0x4
#define IO_MUSTWAIT(e) \
    (((e) == EAGAIN) || ((e) == EWOULDBLOCK) || ((e) == EINPROGRESS))


/* select module */
#ifdef CONFIG_UAIO_SELECT

struct uaio_select;


struct uaio_select *
uaio_select_create(struct uaio* c, size_t maxfileno);


int
uaio_select_destroy(struct uaio* c, struct uaio_select *s);


int
uaio_select_monitor(struct uaio_select *s, struct uaio_task *task, int fd,
        int events);


int
uaio_select_forget(struct uaio_select *s, int fd);


#endif  // CONFIG_UAIO_SELECT
#endif  // CONFIG_UAIO_FDMON
#endif  // CONFIG_UAIO_MODULES


struct uaio*
uaio_create(size_t maxtasks);


int
uaio_destroy(struct uaio* c);


struct uaio_task *
uaio_task_new(struct uaio* c);


int
uaio_task_dispose(struct uaio_task *task);


void
uaio_task_killall(struct uaio* c);


int
uaio_loop(struct uaio* c);


int
uaio_sleep(struct uaio_task *task, unsigned long us);


/* Generic stuff */
#define UAIO_NAME_PASTER(x, y) x ## _ ## y
#define UAIO_NAME_EVALUATOR(x, y)  UAIO_NAME_PASTER(x, y)
#define UAIO_NAME(n) UAIO_NAME_EVALUATOR(UAIO_ENTITY, n)


#define ASYNC void
#define UAIO_AWAIT(task, entity, coro, ...) \
    do { \
        (task)->current->line = __LINE__; \
        if (entity ## _call_new(task, coro, __VA_ARGS__)) { \
            (task)->status = UAIO_TERMINATING; \
        } \
        return; \
        case __LINE__:; \
    } while (0)


#define UAIO_SLEEP(task, us) \
    do { \
        (task)->current->line = __LINE__; \
        if (uaio_sleep(task, us)) { \
            (task)->status = UAIO_TERMINATING; \
        } \
        return; \
        case __LINE__:; \
    } while (0)


#define UAIO_BEGIN(task) \
    switch ((task)->current->line) { \
        case 0:


#define UAIO_FINALLY(task) \
        case -1:; } \
    (task)->status = UAIO_TERMINATED


#define UAIO_RETURN(task) \
    (task)->eno = 0; \
    (task)->status = UAIO_TERMINATING; \
    return


#define UAIO_THROW(task, n) \
    (task)->eno = n; \
    (task)->status = UAIO_TERMINATING; \
    return


#define UAIO_RETHROW(task) \
    (task)->status = UAIO_TERMINATING; \
    return


#define UAIO_HASERROR(task) (task->eno != 0)
#define UAIO_ISERROR(task, e) (UAIO_HASERROR(task) && (task->eno == e))
#define UAIO_CLEARERROR(task) task->eno = 0


#endif  // UAIO_H_
