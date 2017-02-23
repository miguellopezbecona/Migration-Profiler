#include "pages_ops.h"

#include <math.h> // log2

static unsigned int current_step;
const long pagesize = sysconf(_SC_PAGESIZE);
const int expn = log2(pagesize);

void build_page_tables(memory_data_list_t memory_list, map<pid_t, page_table_t> *page_ts){
	int page_node, cpu_node;

	bool is_first_step = current_step == 0;

	// For each element in mem list, we calculate page address, get page node, and then it is added to the new list
	// TODO: do this stuff in mem list to reduce overhead and memory?
	memory_data_cell_t m_cell;
	for(int i=0;i<memory_list.list.size();i++){
		m_cell = memory_list.list[i];

		// This gets page address with offset 0
		long int page_addr = m_cell.addr & ~((long int)(pagesize -1));

		// This gets page number
		long int page_num = m_cell.addr >> expn;

		page_node = get_page_current_node(m_cell.tid, page_addr);
		cpu_node = get_cpu_memory_cell(m_cell.cpu);

		// We skip elements which returned error when getting page node
		if(page_node < 0 || cpu_node == -1)
			continue;

		set_tid_core(m_cell.tid, m_cell.cpu); // Get TID location (core)

		if(is_first_step)
			page_ts->operator[](m_cell.pid).init(m_cell.pid);

		page_ts->operator[](m_cell.pid).add_cell(page_addr, page_node, m_cell.tid, m_cell.latency, m_cell.cpu, cpu_node, m_cell.is_cache_miss());
	}
}

inline int get_page_current_node(pid_t pid, long int pageAddr){
	unsigned long count = 1;
	void **pages = (void **)calloc(1,sizeof(long int *));
	pages[0] = (void *)pageAddr;
	int *status = (int*)calloc(1,sizeof(int));

	//printf("getting page current node for tid %d and pageAddr 0x%016lx\n",pid,(long int)pages[0]);
	numa_move_pages(pid,count,pages,NULL,status,MPOL_MF_MOVE); // If nodes is NULL, in status we obtain the node where the page is
	//printf("%d\n",status[0]);

	return status[0];
}


const char* names[] = {"acs", "min", "avg", "max", "alt"};
FILE *fps[NUM_FILES];
int pages(unsigned int step, vector<pid_t> pids, memory_data_list_t memory_list, map<pid_t, page_table_t> *page_ts){
	current_step = step;

	// Builds tables
	build_page_tables(memory_list, page_ts);

	// Some things are done every ITERATIONS_PER_PRINT iterations
	pid_t main_pid = pids[0];
	if(current_step % ITERATIONS_PER_PRINT == 0){
		#ifdef PRINT_CSVS
		// Opens files
		for(int i=0;i<NUM_FILES;i++){
			char filename[12]; // xxx_yyy.csv
			sprintf(filename, "%s_%d_%d.csv", names[i], main_pid, current_step);
			fps[i] = fopen(filename, "w");

			if(fps[i] == NULL){
				printf("Error opening file %s to log heatmap.\n", filename);
				return -1;
			}
		}

		page_ts->operator[](main_pid).print_heatmaps(fps, NUM_FILES-1);
		page_ts->operator[](main_pid).print_alt_graph(fps[NUM_FILES-1]);

		// Closes files
		for(int i=0;i<NUM_FILES;i++)
			fclose(fps[i]);
		#endif

		//page_ts->operator[](main_pid).print_table1();
		//page_ts->operator[](main_pid).print_table2();

		page_ts->operator[](main_pid).calculate_performance_tid(1000);
		page_ts->operator[](main_pid).calculate_performance_page(1000);
		//page_ts->operator[](main_pid).print_performance();
	}

	return 0;
}

