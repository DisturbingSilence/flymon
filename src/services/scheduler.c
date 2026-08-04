#include "scheduler.h"

static task_t task_table[SCHEDULER_MAX_TASKS] = {};
static uint8_t table_index = 0;
sch_error_t scheduler_add_task(taskentryp_t entry,systime_t period,void* context)
{
    if(table_index >= SCHEDULER_MAX_TASKS) return SCHEDULER_TOO_MANY_TASKS;
    task_table[table_index++] =
    (task_t)
    {
        .entry = entry,
        .period = period,
        .last_run = 0,
        .ctx = context
    };
    return SCHEDULER_OK;
}
void scheduler_run()
{
    while(1)
    {
        systime_t stime = systime_get();
        for(task_t* task = task_table;task < task_table + table_index;task++)
        {
            if(!task->entry) continue;
            if(stime - task->last_run >= task->period)
            {
                task->last_run = stime;
                task->entry(task->ctx);
            }
        }
    }

}
