/**
* Copyright (C) 2008 Happy Fish / YuQing
*
* FastDFS may be copied only under the terms of the GNU General
* Public License V3, which may be found in the FastDFS source kit.
* Please visit the FastDFS Home Page http://www.csource.org/ for more detail.
**/

//work_thread.h

#ifndef _WORK_THREAD_H
#define _WORK_THREAD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <event.h>
#include "fdht_define.h"
#include "task_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

int work_thread_init();
void work_thread_destroy();
int work_notify_task();

#ifdef __cplusplus
}
#endif

#endif
