#ifndef __PAGE_MIGRATION_ALGORITHM_H__
#define __PAGE_MIGRATION_ALGORITHM_H__

//#define PAGE_MIGRATION_OUTPUT

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>

//for numa_move_pages, link with -lnuma
#include <numa.h>
#include <numaif.h>

// Uses page table
#include "page_table.h"

// Needs to know the system
#include "../thread_migration/system_struct.h"

extern int total_thread_migrations;
extern int total_page_migrations;

int migratePages(pid_t pid, page_table_t *page_t);

#endif
