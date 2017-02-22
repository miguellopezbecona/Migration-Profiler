#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Uses page table
#include "../migration/page_table.h"

// Needs to know the system
#include "../migration/system_struct.h"

#include "../migration/migration_cell.h"

typedef struct strategy {
	// Cores function to select the page/thread to migrate and where
	migration_cell_t get_page_to_migrate(page_table_t *page_t);
	migration_cell_t get_thread_to_migrate(pid_t pid, page_table_t *page_t);
} strategy_t;
