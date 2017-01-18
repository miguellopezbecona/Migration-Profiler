#ifndef __PAGES_OPS_H__
#define __PAGES_OPS_H__

#include <unistd.h>

//for move_pages, compile with -lnuma
#include <numa.h>
#include <numaif.h>

/* uses some lists*/
#include "../thread_migration/memory_list.h"
#include "page_table.h"

#define ITERATIONS_PER_PRINT 5

int pages(unsigned int step, memory_data_list_t memory_list, page_table_t *page_t);
inline int get_page_current_node(pid_t pid, long int pageAddr);

#endif
