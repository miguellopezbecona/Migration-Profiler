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
	pid_t pid; // -1 for thread-related elements

	migration_cell(){}
	migration_cell(long int elem, unsigned char dest);
	migration_cell(long int elem, unsigned char dest, pid_t pid);

	bool is_thread_cell() const;
	void perform_page_migration() const;
	void perform_thread_migration() const;
	void perform_migration() const; // Can make private the two methods above
} migration_cell_t;

