#ifndef __THREAD_MIGRATION_AUX_H__
#define __THREAD_MIGRATION_AUX_H__

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sched.h>

/* USES lists*/
#include "pid_list.h"
#include "tid_list.h"
#include "migration_list.h"
#include "migration_table.h"

/* NEEDS TO KNOW THE SYSTEM */
#include "system_struct.h"

#include <vector> // std::vector
using namespace std;

extern core_table_t core_table;

typedef struct target_thread{
	tid_cell_t *tid_c;
	uint32_t target_core;
} target_thread_t;

void init_aux_functions();
void print_target_thread(target_thread_t target);
void set_affinity_error();
unsigned int get_migration_step();
void set_migration_step(unsigned int step);
target_thread_t get_worst_thread_latency(tid_list_t* list);
int remove_core_all_nofixed_tids(int core, tid_list_t* list);
int add_core_all_nofixed_tids(int core, tid_list_t* list);
int pin_tid_to_core(int core, tid_cell_t* tid_c);
int update_affinity_tid(tid_cell_t *tid_c);
int update_affinities_all_tids(tid_list_t* list);
int pin_all_threads(tid_list_t* list);
int pin_all_threads_to_free_cores_and_move(tid_list_t* list);
int pin_all_threads_to_free_cores_and_move_and_free_cores_if_inactive(tid_list_t* list);
int pin_all_threads_with_pid_greater_than_to_free_cores_and_move_and_free_cores_if_inactive(pid_list_t*  pid_l, pid_t pid);
int migrate_pinned_thread_and_free_table(tid_cell_t *tid_c, int target_core);
int interchange_pinned_threads(tid_cell_t *tid1_c, tid_cell_t *tid2_c);
//returns -2 if error, -1 if not free core, core number on success
int migrate_thread_to_free_core(tid_cell_t *tid_c);
int migrate_thread_to_random_free_core(tid_cell_t *tid_c);
tid_cell_t * get_random_tid(tid_list_t* list);

/*
 * Thread lottery procedures
*/
int migrate(tid_cell_t *tid_c, migration_cell_t *target_migration);
//maybe move?
#define TICKETS_MEM_CELL_NO_VALID -1
#define TICKETS_MEM_CELL_WORSE 1
#define TICKETS_MEM_CELL_NO_DATA 2
#define TICKETS_MEM_CELL_BETTER 4
#define TICKETS_MEM_CELL_WORSE_2 1
#define TICKETS_MEM_CELL_NO_DATA_2 2
#define TICKETS_MEM_CELL_BETTER_2 4
#define TICKETS_FREE_CORE 3
//current_mem_cell is wanted to avoid having to recalculate
int get_lottery_weight_for_mem_cell(int mem_cell,int current_cell, tid_cell_t *thread_cell);
//This uses the second values, it is the same as before, only used if different values for the two threads are wanted 
int get_lottery_weight_for_mem_cell_2(int mem_cell,int current_cell, tid_cell_t *thread_cell);
//uses only migration table
migration_list_t migration_destinations_wweight(tid_cell_t *target_thread, pid_list_t* pid_m);

#endif
