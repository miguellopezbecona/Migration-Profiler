#pragma once


/* USES lists*/
#include "pid_list.h"

/* NEEDS TO KNOW THE SYSTEM */
#include "system_struct.h"

typedef struct core_table {
	pid_t list[SYS_NUM_OF_CORES];

	core_table(){}

	int init();
	void print();
	int free_tid(int core, pid_t tid);
	int set_tid_to_core(int core, pid_t tid);
	int find_first_free_core();
	int find_next_free_core(int core);
	int get_random_free_core();
	int pin_tid_to_core_and_set_table(int core, tid_cell_t *tid_c);
	int pin_tid_to_first_free_core(tid_cell_t *tid_c);
	int pin_tid_to_core_or_first_free_core(int core, tid_cell_t *tid_c);

	bool is_core_free(int core);
	pid_t get_tid_in_core(int core);
} core_table_t;

