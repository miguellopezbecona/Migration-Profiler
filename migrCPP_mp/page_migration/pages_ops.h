#pragma once

#include <unistd.h>

// For move_pages, compile with -lnuma
#include <numa.h>
#include <numaif.h>

// Uses some structures
#include "../thread_migration/memory_list.h"
#include "page_table.h"

//#define PRINT_CSVS
#define ITERATIONS_PER_PRINT 5

#define NUM_FILES 5

int pages(unsigned int step, pid_t pid, memory_data_list_t memory_list, page_table_t *page_t);
inline int get_page_current_node(pid_t pid, long int pageAddr);

