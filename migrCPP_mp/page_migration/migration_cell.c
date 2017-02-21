#include "migration_cell.h"

void migration_cell_t::perform_page_migration(pid_t pid){
	void **page = (void **)calloc(1,sizeof(long int *));
	page[0] = (void*) &elem.page_addr;
	int *status = (int *)calloc(1,sizeof(int));

	// Key system call: numa_move_pages
	int rc = numa_move_pages(pid, 1, page, (int*) &dest.mem_node, status, MPOL_MF_MOVE);
	if (rc < 0){
		printf("Move pages did not work: %s\n", strerror(errno));
		exit(1);
	}

	total_page_migrations++;

	#ifdef MIGRATION_OUTPUT
		printf("Migrated page number %016lx to node %d, status=%d", (long unsigned int) elem.page_addr, dest.mem_node, status[0]);
		if(status[0] < 0)
			printf(", err: %s", strerror(-status[0]));
		printf("\n");
	#endif
}

void set_affinity_error(){
	switch(errno){
		case EFAULT:
			printf("Error setting affinity: A supplied memory address was invalid\n");
			break;
		case EINVAL:
			printf("Error setting affinity: The affinity bitmask mask contains no processors that are physically on the system, or cpusetsize is smaller than the size of the affinity mask used by the kernel\n");
			break;
		case EPERM:
			printf("Error setting affinity: The calling process does not have appropriate privileges\n");
			break;
		case ESRCH:
			printf("Error setting affinity: The process whose ID is pid could not be found\n");
			break;
	}
}

void migration_cell_t::perform_thread_migration(){
	cpu_set_t affinity;
	sched_getaffinity(elem.tid, sizeof(cpu_set_t), &affinity);

	CPU_ZERO(&affinity);
	CPU_SET(dest.core, &affinity);

	if(sched_setaffinity(elem.tid, sizeof(cpu_set_t), &affinity))
		set_affinity_error();

	#ifdef MIGRATION_OUTPUT
		printf("Migrated thread %d to core %d\n", elem.tid, dest.core);
	#endif

	total_thread_migrations++;
}
