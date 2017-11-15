#pragma once

#include <unistd.h>

// For move_pages, compile with -lnuma
#include <numa.h>
#include <numaif.h>

// Uses some structures
#include "memory_list.h"
#include "inst_list.h"
#include "page_table.h"

//#define PERFORMANCE_OUTPUT

//#define PRINT_CSVS
#define ITERATIONS_PER_PRINT 5

#define NUM_FILES 5

int pages(unsigned int step, set<pid_t> pids, memory_data_list_t m_list, inst_data_list i_list, map<pid_t, page_table_t> *page_ts);
inline int get_page_current_node(pid_t pid, long int pageAddr);

