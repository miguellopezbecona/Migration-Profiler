#ifndef __MIGRATION_ALGORITHM_H__
#define __MIGRATION_ALGORITHM_H__

//#define THREAD_MIGRATION_OUTPUT

/* USES lists*/
#include "../common_util.h"
#include "thread_migration_aux.h"
#include "pid_list.h"
#include "migration_list.h"

/* NEEDS TO KNOW THE SYSTEM */
#include "system_struct.h"

extern int total_thread_migrations;

int init_migration_algorithm();

//this snapshot is an array with SYS_NUM_OF_CORES elements
int migrateThreads(pid_t pid, pid_list_t* pid_m, unsigned int step);

int undo_last_migration();

#endif
