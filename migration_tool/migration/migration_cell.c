#include "migration_cell.h"

int migration_cell_t::total_thread_migrations = 0;
int migration_cell_t::total_page_migrations = 0;

migration_cell_t::migration_cell(long int elem, short dest) {
	this->elem = elem;
	this->dest = dest;
	this->prev_dest = -1;
	this->pid = -1;
	this->thread_cell = true; // If we don't use a PID, it HAS to be a thread cell
}

migration_cell_t::migration_cell(long int elem, short dest, pid_t pid, bool thread_cell) {
	this->elem = elem;
	this->dest = dest;
	this->prev_dest = -1;
	this->pid = pid;
	this->thread_cell = thread_cell;
}

migration_cell_t::migration_cell(long int elem, short dest, short prev_dest, pid_t pid, bool thread_cell) {
	this->elem = elem;
	this->dest = dest;
	this->prev_dest = prev_dest;
	this->pid = pid;
	this->thread_cell = thread_cell;
}

bool migration_cell_t::is_thread_cell() const {
	return thread_cell;
}

// It moves only one page at once, could be arranged to move more
int migration_cell_t::perform_page_migration() const {
	void **page = (void **)calloc(1,sizeof(long int *));
	page[0] = (void*) elem;
	int dest_int = dest, status;

	// Key system call: numa_move_pages
	int ret = numa_move_pages(pid, 1, page, &dest_int, &status, MPOL_MF_MOVE);
	if (ret < 0){
		printf("Move pages did not work: %s\n", strerror(errno));
		return errno;
	}

	total_page_migrations++;

	#ifdef PG_MIGR_OUTPUT
		printf("Migrated page number %016lx to node %d, status=%d", (long unsigned int) elem, dest, status);
		if(status < 0)
			printf(", err: %s", strerror(-status));
		printf("\n");
	#endif

	return ret;
}

int migration_cell_t::perform_thread_migration() const {
	int ret = system_struct_t::set_tid_cpu((pid_t) elem, dest, true);

	#ifdef TH_MIGR_OUTPUT
	printf("Migrated thread %d to CPU %d\n", (pid_t) elem, dest+1); // +1 only for demo!
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

void migration_cell_t::interchange_dest() {
	short aux = dest;
	dest = prev_dest;
	prev_dest = aux;
}

void migration_cell_t::print() const {
	const char* const types[] = {"Page", "Thread"};
	const char* const elems[] = {"memory page", "TID"};
	const char* const to_migrates[] = {"memory node", "CPU"};

	bool is_thread = is_thread_cell();

	printf("%s migration cell. %s %lu to be migrated to %s %d.", types[is_thread], elems[is_thread], elem, to_migrates[is_thread], dest); // dest+1 for demo only!

	if(pid > 0)
		printf(" PID: %d.", pid);
	if(prev_dest > -1)
		printf(" It was in %s %d.", to_migrates[is_thread], prev_dest); // prev_dest+1 for demo only!
	printf("\n");
}
