#include "migration_cell.h"

migration_cell_t::migration_cell(long int elem, unsigned char dest) {
	this->elem = elem;
	this->dest = dest;
}

// It moves only one page at once, could be arrange to move more
void migration_cell_t::perform_page_migration(pid_t pid) const {
	void **page = (void **)calloc(1,sizeof(long int *));
	page[0] = (void*) &elem;
	int *status = (int *)calloc(1,sizeof(int));

	// Key system call: numa_move_pages
	int rc = numa_move_pages(pid, 1, page, (int*) &dest, status, MPOL_MF_MOVE);
	if (rc < 0){
		printf("Move pages did not work: %s\n", strerror(errno));
		return;
	}

	total_page_migrations++;

	#ifdef MIGRATION_OUTPUT
		printf("Migrated page number %016lx to node %d, status=%d", (long unsigned int) elem, dest, status[0]);
		if(status[0] < 0)
			printf(", err: %s", strerror(-status[0]));
		printf("\n");
	#endif
}

void set_affinity_error(pid_t tid){
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
			printf("Error setting affinity: The process whose ID is %d could not be found\n", tid);
			break;
	}
}

void migration_cell_t::perform_thread_migration() const {
	cpu_set_t affinity;
	sched_getaffinity(elem, sizeof(cpu_set_t), &affinity);

	CPU_ZERO(&affinity);
	CPU_SET(dest, &affinity);

	if(sched_setaffinity(elem, sizeof(cpu_set_t), &affinity)){
		set_affinity_error(elem);
		return;
	}

	#ifdef MIGRATION_OUTPUT
		printf("Migrated thread %d to core %d\n", (int) elem, dest);
	#endif

	total_thread_migrations++;
}
