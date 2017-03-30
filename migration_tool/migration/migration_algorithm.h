#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Uses page table
#include "page_table.h"

// Needs to know the system
#include "system_struct.h"

//#define MIGRATION_OUTPUT

extern int total_thread_migrations;
extern int total_page_migrations;

int perform_migration_strategy(page_table_t *page_t); // For a one process-only strategy
int perform_migration_strategy(map<pid_t, page_table_t> *page_ts); // For a global strategy

