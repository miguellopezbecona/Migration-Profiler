#include "migration_cell.h"

migration_cell_t::migration_cell(long int elem, unsigned char dest, pid_t pid, bool thread_cell) {
	this->elem = elem;
	this->dest = dest;
	this->pid = pid;
	this->thread_cell = thread_cell;
}

bool migration_cell_t::is_thread_cell() const {
	return thread_cell;
}

// It moves only one page at once, could be arranged to move more
int migration_cell_t::perform_page_migration() const {
	void **page = (void **)calloc(1,sizeof(long int *));
	page[0] = (void*) &elem;
	int dest_int = dest, status;

	// Key system call: numa_move_pages
	int ret = numa_move_pages(pid, 1, page, &dest_int, &status, MPOL_MF_MOVE);
	if (ret < 0){
		printf("Move pages did not work: %s\n", strerror(errno));
		return errno;
	}

	total_page_migrations++;

	#ifdef MIGRATION_OUTPUT
		printf("Migrated page number %016lx to node %d, status=%d", (long unsigned int) elem, dest, status);
		if(status < 0)
			printf(", err: %s", strerror(-status));
		printf("\n");
	#endif

	return ret;
}

// Auxiliar function
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

int migration_cell_t::perform_thread_migration() const {
	cpu_set_t affinity;
	sched_getaffinity(elem, sizeof(cpu_set_t), &affinity);
	int ret = 0;

	CPU_ZERO(&affinity);
	CPU_SET(dest, &affinity);

	if((ret = sched_setaffinity(elem, sizeof(cpu_set_t), &affinity))){
		set_affinity_error(elem);
		return errno;
	}

	#ifdef MIGRATION_OUTPUT
		printf("Migrated thread %d to core %d\n", (int) elem, dest);
	#endif

	total_thread_migrations++;

	return ret;
}

int migration_cell_t::perform_migration() const {
	if(is_thread_cell())
		return perform_thread_migration();
	else
		return perform_page_migration();
}

void migration_cell_t::print() const {
	if(is_thread_cell())
		printf("Thread migration cell. %lu TID, %d core to migrate, %d PID.\n", elem, dest, pid);
	else
		printf("Page migration cell. %lu memory page, %d memory node to migrate, %d PID.\n", elem, dest, pid);
}
