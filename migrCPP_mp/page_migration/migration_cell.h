#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>

//for numa_move_pages, link with -lnuma
#include <numa.h>
#include <numaif.h>

#include "migration_algorithm.h"

// We need to know what to migrate and where
typedef struct migration_cell {
	union {
		int tid;
		long int page_addr;
	} elem;
	union {
		unsigned char core;
		unsigned char mem_node;
	} dest;

	void perform_page_migration(pid_t pid);
	void perform_thread_migration();
} migration_cell_t;

