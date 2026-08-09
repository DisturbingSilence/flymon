#pragma once
#include <drivers/systime.h>

#define SCHEDULER_MAX_TASKS 32
typedef void(*taskentryp_t)(void* ctx);
typedef struct
{
    taskentryp_t entry;
    systime_t period;
    systime_t last_run;
    void* ctx;
} task_t;

int scheduler_add_task(taskentryp_t entry,systime_t period,void* context);
void scheduler_run();
