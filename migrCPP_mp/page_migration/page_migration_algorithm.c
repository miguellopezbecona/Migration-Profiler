//#define _GNU_SOURCE
#include "page_migration_algorithm.h"

int total_page_migrations = 0;

//Do better eventually, this is just for checking it works
/*
int performPageMigrations(pid_t pid, page_list_t *page_l){
	page_cell_t *p_cell;

	int tid;
	unsigned long count;
	void **pages2migrate;
	int *nodes;
	int *status;
	int rc;

	count = page_l->list.size();
	//Worst-case scenario: all pages must be migrated.
	pages2migrate = (void **)calloc(count,sizeof(long int *));
	nodes = (int *)calloc(count,sizeof(const int));
	status = (int *)calloc(count,sizeof(int));

	if (!pages2migrate || !status || !nodes)
	{
		printf("Unable to allocate memory to perform page migrations\n");
		exit(1);
	}


	// To fill arrays with info about which pages to move and where.
	count = 0;

	// We get information (latency and id) about the group with the minimum latency (in average) to access its pages
	double* best_node_info = page_l->get_lowest_mean_accesed_by_itself_node();
	double lowest_mean_latency = best_node_info[0];
	int migrate_to = (int) best_node_info[1];
	//printf("%.2f %d\n", lowest_mean_latency, node);

	double min_average_latency;
	for(int i=0;i<page_l->list.size();i++){
		p_cell = &page_l->list[i];

		// We skip pages which already reside in the chosen one
		if(p_cell->current_node == migrate_to)
			continue;

		// The algorithm considers that if T_q < min_average_latency(p_i) then the page p_i should be migrated to group G_q
		min_average_latency = p_cell->get_minimum_mean_latency();
		if(lowest_mean_latency < min_average_latency){
			//printf("Check m_t: %d, c_n: %d\n", migrate_to, p_cell->current_node);

			pages2migrate[count] = (void *)p_cell->page_addr;
			nodes[count] = migrate_to;
			// Let's be optimistic and assume the migration goes right
			p_cell->current_node = migrate_to;
			//update count
			count++;
		}
	}

	if(count==0){
		#ifdef PAGE_MIGRATION_OUTPUT
			printf("NO PAGES TO MIGRATE\n");
		#endif
		return 1;
	}

	#ifdef PAGE_MIGRATION_OUTPUT
		printf("Going to migrate %lu pages:\n",count);
	#endif

	total_page_migrations += count;

	// And now, the actual work: let's move pages
	rc = numa_move_pages(pid, count, pages2migrate, nodes, status, MPOL_MF_MOVE);
	if (rc < 0){
		//perror("move_pages");
		//gettimeofday(&total_end, NULL);
		//printf("Real execution time from last move_pages: %lf seconds\n",((double)total_end.tv_sec + (double)total_end.tv_usec/1000000) - \((double)total_start.tv_sec + (double)total_start.tv_usec/1000000));
		printf("move pages did not work: %s\n", strerror(errno));
		exit(1);

	}


	// If returned status[i]= -ENOENT (i.e. -2), the page is no longer allocated in memory (check move_pages.c example in libnuma)
	#ifdef PAGE_MIGRATION_OUTPUT
	for (int i = 0; i < count; i++){
		printf("\tPageMigrated: addr=0x%016lx node=%d status=%d", (long unsigned int) pages2migrate[i], nodes[i], status[i]);
		if(status[i] < 0)
			printf(", err: %s", strerror(-status[i]));
		printf("\n");
	}
	printf("\n");
	#endif

	return 0;
}



int migratePages(pid_t pid, page_list_t *page_l){
	//simple_algorithm(pid, pid_l, snapshot_l_all);
	performPageMigrations(pid, page_l);
}
*/

/*** Version with page_table ****/
int performPageMigrations(pid_t pid, page_table_t *page_t){
	return 0;
}


int migratePages(pid_t pid, page_table_t *page_t){
	performPageMigrations(pid, page_t);
}
