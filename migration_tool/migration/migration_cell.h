#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>

// For numa_move_pages, link with -lnuma
#include <numa.h>
#include <numaif.h>

#include "migration_algorithm.h"

// We need to know what to migrate and where
typedef struct migration_cell {
	long int elem; // Core or page address
	unsigned char dest; // Mem node for pages or core for TIDs

	migration_cell(){}
	migration_cell(long int elem, unsigned char dest);
	void perform_page_migration(pid_t pid) const;
	void perform_thread_migration() const;
} migration_cell_t;

