#pragma once

#include "../sample_data.h"
#include "memory_list.h"
#include "inst_list.h"

// Pages
#include "pages_ops.h"
#include "migration_algorithm.h"

#define ACTIVATE_THREAD_MIGRATION 0
#define ACTIVATE_PAGE_MIGRATION 1

int process_my_pebs_sample(my_pebs_sample_t sample);
int begin_migration_process(vector<pid_t> pids, int do_thread_migration, int do_page_migration);

