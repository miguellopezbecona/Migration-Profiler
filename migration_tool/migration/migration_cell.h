#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// For numa_move_pages
#include <numa.h>
#include <numaif.h>

#include "system_struct.h"

//#define PG_MIGR_OUTPUT
#define TH_MIGR_OUTPUT

// We need to know what to migrate and where
typedef struct migration_cell {
	static int total_thread_migrations;
	static int total_page_migrations;

	long int elem; // Core or page address
	short dest; // Mem node for pages or core for TIDs
	short prev_dest; // Useful for undoing migrations
	pid_t pid;
	bool thread_cell;

	migration_cell(){}
	migration_cell(long int elem, short dest, pid_t pid, bool thread_cell);
	migration_cell(long int elem, short dest, short prev_dest, pid_t pid, bool thread_cell);

	bool is_thread_cell() const;
	int perform_page_migration() const;
	int perform_thread_migration() const;
	int perform_migration() const; // Can make private the two methods above
	void interchange_dest();
	void print() const;
} migration_cell_t;

