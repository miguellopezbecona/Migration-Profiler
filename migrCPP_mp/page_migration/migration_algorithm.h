#pragma once

#define MIGRATION_OUTPUT

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Uses page table
#include "page_table.h"

// Needs to know the system
#include "../thread_migration/system_struct.h"

extern int total_thread_migrations;
extern int total_page_migrations;

int perform_migration_strategy(pid_t pid, page_table_t *page_t);

