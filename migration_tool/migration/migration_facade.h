#pragma once

#include "../sample_data.h"
#include "memory_list.h"
#include "inst_list.h"
#include "pages_ops.h"
#include "migration_algorithm.h"

// If you only want to use this tool for profiling, uncomment the following:
#define JUST_PROFILE

// How many samples do you want to log in the CSV files in JUST_PROFILE mode?
#define ELEMS_PER_PRINT 100

#define ACTIVATE_THREAD_MIGRATION 0
#define ACTIVATE_PAGE_MIGRATION 1

void add_data_to_list(my_pebs_sample_t sample);
int begin_migration_process(int do_thread_migration, int do_page_migration);
void clean_migration_structures();

void work_with_fake_data();

