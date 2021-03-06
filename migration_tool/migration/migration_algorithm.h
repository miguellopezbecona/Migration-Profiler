#pragma once

// Uses page table
#include "page_table.h"

//#define USE_RAND_ST
//#define USE_FIRST_ST
//#define USE_ANNEA_ST
//#define USE_GEN_ST
//#define USE_ENER_ST

extern unsigned int step; // Iteration number

int perform_migration_strategy(page_table_t *page_t); // For a one process-only strategy
int perform_migration_strategy(map<pid_t, page_table_t> *page_ts); // For a global strategy

