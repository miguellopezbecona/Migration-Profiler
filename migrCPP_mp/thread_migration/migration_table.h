#ifndef __MIGRATION_TABLE_H__
#define __MIGRATION_TABLE_H__


/* USES lists*/
#include "pid_list.h"

/* NEEDS TO KNOW THE SYSTEM */
#include "system_struct.h"

//to save which tid are set to which core (limit of two for now)
#define MAX_THREADS_PER_CORE 1
//tid_cell_t * core_table[SYS_NUM_OF_CORES][MAX_THREADS_PER_CORE];

//max number of threads in total
//#define MAX_THREADS 32

int init_migration_table();
void print_core_table();
int free_table_core(int core, pid_t tid);
//returns place inserted, or -1 if no free core
int set_table_core(int core, pid_t tid);
//returns -1 if not free core
int find_first_free_core();
int find_next_free_core(int core);
int get_random_free_core();
int pin_tid_to_core_and_set_table(int core, tid_cell_t *tid_c);
//returns -1 if no free spots, can be MAX_THREADS_PER_CORE
int pin_tid_to_first_free_core(tid_cell_t *tid_c);
//returns -1 if no free spots, can be MAX_THREADS_PER_CORE
int pin_tid_to_core_or_first_free_core(int core, tid_cell_t *tid_c);

int is_core_free(int core);
pid_t get_tid_in_core(int core, int thread_position);


#endif

