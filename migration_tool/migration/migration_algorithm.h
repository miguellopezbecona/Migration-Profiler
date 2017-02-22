#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Uses page table
#include "page_table.h"

// Needs to know the system
#include "system_struct.h"

#define MIGRATION_OUTPUT

extern int total_thread_migrations;
extern int total_page_migrations;

int perform_migration_strategy(pid_t pid, page_table_t *page_t);

