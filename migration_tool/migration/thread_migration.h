#include <stdio.h>

#include "../common_util.h"
#include "memory_list.h"
#include "inst_list.h"

// Pages
#include "pages_ops.h"
#include "migration_algorithm.h"

#define ACTIVATE_THREAD_MIGRATION 0
#define ACTIVATE_PAGE_MIGRATION 1

/*
 * RETURN CODES
*/
#define RET_NO_INST -2
#define RET_NO_MEM 2 //This may not be fatal

int process_my_pebs_sample(pid_t pid, my_pebs_sample_t sample);

//Does not work if no inst data, RET_NO_INST, works without mem data, RET_NO_MEM
int do_migration_and_clear_temp_list(pid_t pid, int do_thread_migration, int do_page_migration);

