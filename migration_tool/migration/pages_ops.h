#pragma once

#include <unistd.h>

// For move_pages, compile with -lnuma
#ifndef FAKE_DATA
#include <numa.h>
#include <numaif.h>
#endif

// Uses some structures
#include "memory_list.h"
#include "inst_list.h"
#include "page_table.h"

#include "migration_algorithm.h" // For checking strategy macros

//#define PERFORMANCE_OUTPUT

//#define PRINT_HEATMAPS
const int ITERATIONS_PER_HEATMAP_PRINT = 5;
const int NUM_FILES = 5;

int pages(set<pid_t> pids, memory_data_list_t m_list, inst_data_list i_list, map<pid_t, page_table_t> *page_ts);
inline int get_page_current_node(pid_t pid, long int pageAddr);

