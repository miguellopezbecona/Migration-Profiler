#include "page_table.h"

/*** table_cell_t ***/
table_cell_t::table_cell(int latency, bool is_cache_miss){
	latencies.push_back(latency);
	cache_misses = (unsigned) is_cache_miss;
}

void table_cell_t::update(int latency, bool is_cache_miss){
	latencies.push_back(latency);
	cache_misses += (unsigned) is_cache_miss;
}

void table_cell_t::print(){
	printf("NUM_ACC %lu, MEAN_LAT %.2f, CACH_MIS: %u\n", latencies.size(), accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size(), cache_misses);
}

/*** page_table_t ***/
page_table_t::page_table(pid_t p) : page_node_table(system_struct_t::NUM_OF_MEMORIES) {
	pid = p;
	table.resize(system_struct_t::NUM_OF_CPUS);
}

// Destructor. We clear everything
page_table_t::~page_table(){
	// Table itself
	for(column& c : table)
		c.clear();
	table.clear();

	// Vectors from maps, and maps itself
	for(auto& it : page_node_map)
		it.second.acs_per_node.clear();
	page_node_map.clear();

	for(auto& it : perf_per_tid){
		rm3d_data_t* pd = &it.second;
		pd->insts.clear();
		pd->reqs.clear();
		pd->times.clear();
		pd->v_perfs.clear(); 
	}
	perf_per_tid.clear();

	table.clear();
	uniq_addrs.clear();
	tid_index.clear();
}

int page_table_t::add_cell(long int page_addr, int current_node, pid_t tid, int latency, int cpu, int cpu_node, bool is_cache_miss){
	table_cell_t *cell = get_cell(page_addr, tid);

	if(cell == NULL) {
		table_cell_t aux(latency, is_cache_miss);
		uniq_addrs.insert(page_addr); // Adds address to the set, won't be inserted if already added by other threads

		// If TID does not exist in map, we associate a index to it
		if(tid_index.count(tid) == 0){
			tid_index[tid] = tid_index.size();

			// Resizes vector (row) if necessary
			size_t ts = table.size();
			if(ts == tid_index.size())
				table.resize(2*ts);
		}
		int pos = tid_index[tid];
		table[pos][page_addr] = aux;
	} else
		cell->update(latency, is_cache_miss);
	
	// Creates/updates association in page node map
	pg_perf_data_t *pd = &page_node_map[page_addr];
	pd->last_cpu_access = cpu;
	pd->current_node = current_node;
	pd->acs_per_node[cpu_node]++;

	// Creates/updates association in experimental table
	// [TOTHINK]: right now, we are storing accesses and latencies using the node where the page is, maybe it should be done regarding the CPU source of the access?
	page_node_table.add_data(page_addr, current_node, latency);

	return 0;
}

bool page_table_t::contains_addr(long int page_addr, int tid){
	if(tid_index.count(tid) == 0)
		return false;

	int pos = tid_index[tid];
	return table[pos].count(page_addr) > 0;
}

vector<int> page_table_t::get_latencies_from_cell(long int page_addr, int tid){
	if(contains_addr(page_addr,tid)){
		int pos = tid_index[tid];
		return table[pos][page_addr].latencies;
	} else {
		vector<int> v;
		return v;
	}
}

// Returns NULL if no association
table_cell_t* page_table_t::get_cell(long int page_addr, int tid){
	if(contains_addr(page_addr,tid)){
		int pos = tid_index[tid];
		return &table[pos][page_addr];
	} else
		return NULL;
}

// Somewhat ugly. You can't just remove a single TID without iterating because you need to change the following indexes
void page_table_t::remove_tid(pid_t tid){
	bool erased = false;

	for(map<int, short>::iterator it = tid_index.begin(); it != tid_index.end(); ++it) {
		pid_t it_tid = it->first;
		int pos = it->second;

		// Removing a thread (row) implies deleting the map association (also in system struct), deleting the element in the vector, and updating following positions
		if(tid == it_tid){
			printf("TID %d will be removed from the table.\n", tid);
			tid_index.erase(it);
			++it;
			table.erase(table.begin() + pos - erased);
			erased = true;
			system_struct_t::remove_tid(tid, false);
		}
		else
			tid_index[it_tid] = pos - erased;
	}
}

// Also unpins inactive threads
void page_table_t::remove_finished_tids(){
	unsigned int erased = 0;

	for(map<int, short>::iterator it = tid_index.begin(); it != tid_index.end(); ++it) {
		pid_t tid = it->first;
		int pos = it->second;

		// Removing a thread (row) implies deleting the map association (also in system struct), deleting the element in the vector, and updating following positions
		if(!is_tid_alive(pid, tid)){
			printf("TID %d is dead. It will be removed from the table.\n", tid);
			tid_index.erase(it);
			++it;
			table.erase(table.begin() + pos - erased);
			erased++;
			system_struct_t::remove_tid(tid, false);
		}
		else {
			if(!perf_per_tid[tid].active)
				system_struct_t::remove_tid(tid, true);
			
			tid_index[tid] = pos - erased;
		}
	}
}

void page_table_t::print() {
	printf("Page table for PID: %d\n", pid);

	// Prints each row (TID)
	for(auto const & t_it : tid_index) {
		int tid = t_it.first;
		int pos = t_it.second;

		printf("TID: %d\n", tid);
		if(table[pos].empty())
			printf("\tEmpty.\n");
		
		// Prints each cell (TID + page address)
		for(auto const & it : table[pos]) {
			long int page_addr = it.first;
			table_cell_t cell = it.second;
			
			printf("\tPAGE_ADDR: %lx, CURRENT_NODE: %d, ", page_addr, page_node_map[page_addr].current_node);
			cell.print();
		}
		printf("\n");
	}
}

void page_table_t::print_heatmaps(FILE **fps, int num_fps){
	// This is for writing number of accesses by each thread in the last row
	int accesses[tid_index.size()];
	memset(accesses, 0, sizeof(accesses));

	// First line of each CSV file: column header
	for(int i=0;i<num_fps;i++){
		// Rowname (first column)
		fprintf(fps[i], "addr,");

		// Threads' ID
		for(auto const & t_it : tid_index)
			fprintf(fps[i], "T_%d,", t_it.first);
		fprintf(fps[i], "\n");
	}

	// Each unique page address will write a row with the data for each thread
	for (long int const & addr : uniq_addrs){

		// Writes address as row name (first column)
		for(int i=0;i<num_fps;i++)
			fprintf(fps[i], "%lx,", addr);

		// Writes data for each thread
		for(auto const & t_it : tid_index) {
			vector<int> l = get_latencies_from_cell(addr, t_it.first);

			// No data
			if(l.empty()){
				for(int j=0;j<num_fps;j++)
					fprintf(fps[j], "-1,");
				continue;
			}

			accesses[t_it.second] += l.size();

			// Calculates data
			int num_accesses = l.size();
			int min_latency = *(min_element(l.begin(), l.end()));
			double mean_latency = accumulate(l.begin(), l.end(), 0.0) / num_accesses;
			int max_latency = *(max_element(l.begin(), l.end()));

			// Prints data to files
			fprintf(fps[0], "%d,", num_accesses);
			fprintf(fps[1], "%d,", min_latency);
			fprintf(fps[2], "%.2f,", mean_latency);
			fprintf(fps[3], "%d,", max_latency);
		}

		for(int j=0;j<num_fps;j++)
			fprintf(fps[j], "\n");
	}

	// After all addresses (rows) are processed, we print number of accesses by each thread in the last row
	for(int i=0;i<num_fps;i++){
		fprintf(fps[i], "num_accesses,"); // Rowname
		for(size_t j=0;j<tid_index.size();j++)
			fprintf(fps[i], "%d,", accesses[j]);
	}

}

// Page vs how many different threads accessed to it?
void page_table_t::print_alt_graph(FILE *fp){
	// First line of the CSV file: column header
	fprintf(fp, "addr,threads_accessed\n");

	// Each unique page address will write a row with the data for each thread
	for (long int const & addr : uniq_addrs){
		int threads_accessed = 0;		

		// A thread has accessed to the page if its cell is not null
		for(auto const & t_it : tid_index)
			threads_accessed += ( get_cell(addr, t_it.first) != NULL );

		// Prints row (row name and data) to file
		fprintf(fp, "%lx,%d\n", addr, threads_accessed);
	}
}

/** The goal of these functions is to update page_node_map and tid_node_map so:
	  - We get from each memory page:
	    -- How many different threads accessed to it.
	    -- How many different threads accessed to it with a latency bigger than a threshold.
	    -- The average latency from those accesses.
	  - We get from each thread:
	    -- How many different memory pages it had accessed to.
	    -- How many different memory pages it had accessed with a latency bigger than a threshold to.
	    -- The average latency from those accesses.
	This info will be crucial to design a migration strategy.
	This implies looping over the table.
*/
// Fills only page_node_map
void page_table_t::calculate_performance_page(int threshold){
	// Loops over memory pages
	for(long int const & addr : uniq_addrs){
		vector<int> l_thres;
		int threads_accessed = 0;
		int num_acs_thres = 0;

		// A thread has accessed to the page if its cell is not null
		for(auto const & t_it : tid_index) {
			vector<int> l = get_latencies_from_cell(addr, t_it.first);
			
			// No data
			if(l.empty())
				continue;

			threads_accessed++;

			// Calculates if there is at least a latency bigger than the threshold and keeps them
			bool upper_thres = 0;
			for(size_t j=0;j<l.size();j++){
				if(l[j] > threshold) {
					l_thres.push_back(l[j]);
					upper_thres = true;
				}
			}
			num_acs_thres += upper_thres;
			
		}

		// Updates data in map
		pg_perf_data_t *cell = &page_node_map[addr];
		cell->num_uniq_accesses = threads_accessed;
		cell->num_acs_thres = num_acs_thres;

		if(!l_thres.empty()){
			cell->median_latency = get_median_from_list(l_thres);
			cell->min_latency = *(min_element(l_thres.begin(), l_thres.end()));
			cell->max_latency = *(max_element(l_thres.begin(), l_thres.end()));
		}
	}
}

void page_table_t::print_performance() const {
/*	// Not interesting right now
	for (auto const & it : page_node_map){
		printf("%lx: ", it.first);
		it.second.print();
	}
	
	printf("\n");
*/

	// Óscar's
/*
	for (auto const & it : perf_per_tid){
		printf("Per-core performance for thread %d:\n", (int) it.first);
		it.second.print();
	}
*/

	// Experimental table
	printf("Page table por PID: %d\n", pid);
	page_node_table.print();
}

// Since pages are associated to processes rather to global system, this step is necessary to have consistent locations
void page_table_t::update_page_locations(vector<migration_cell_t> pg_migr){
	for(migration_cell_t const & pgm : pg_migr){
		long int addr = pgm.elem;
		short new_dest = pgm.dest;
		page_node_map[addr].current_node = new_dest;
	}
}

/*** Functions made for replicating Óscar's work ***/
vector<int> page_table_t::get_lats_for_tid(pid_t tid){
	vector<int> v;

	int pos = tid_index[tid];
	for (auto const & it : table[pos]){
		vector<int> ls = it.second.latencies;
		v.insert(v.end(), ls.begin(), ls.end()); // Appends latencies to main list
	}

	return v;
}

// Gets the mean of the number of accesses of all pages for the whole table
double page_table_t::get_mean_acs_to_pages(){
	vector<short> v;

	// We collect the sums of the accesses per node for each page
	for (auto const & it : page_node_map){
		vector<unsigned short> vaux = it.second.acs_per_node;
		int num_acs = accumulate(vaux.begin(), vaux.end(), 0);
		v.push_back(num_acs);
	}

	// ... and then we calculate the mean
	return accumulate(v.begin(), v.end(), 0.0) / v.size();
}

// Gets the mean of the latencies of all pages for the whole table
double page_table_t::get_mean_lat_to_pages(){
	vector<int> v;
	v.reserve(2*uniq_addrs.size());

	/// We collect the means of the latencies for each page

	// We loop over TIDs
	for(auto const & t_it : tid_index) {
		vector<int> ls = get_lats_for_tid(t_it.first);
		v.insert(v.end(), ls.begin(), ls.end()); // Appends latencies to main list
	}

	// ... and then we calculate the total mean for all the table
	return accumulate(v.begin(), v.end(), 0.0) / v.size();
}

void page_table_t::add_inst_data_for_tid(pid_t tid, int core, long int insts, long int req_dr, long int time) {
	perf_per_tid[tid].add_data(core, insts, req_dr, time);
}

// Needs getting the mean latency for each TID. Maybe we could write a function to get all the latencies for a given TID so get_mean_lat_to_pages() could reuse that
void page_table_t::calc_perf() {
	// We loop over TIDs
	for(auto const & t_it : tid_index) {
		pid_t tid = t_it.first;

		if(!perf_per_tid[tid].active) // Only for active
			continue;

		vector<int> v;
		int pos = t_it.second;

		// We get latencies for each cell for that TID
		for(auto const & it : table[pos]) {
			table_cell_t cell = it.second;
			vector<int> ls = cell.latencies;
			v.insert(v.end(), ls.begin(), ls.end()); // Appends latencies to main list
		}

		// ... and then we calculate the total mean for all the TID
		double mean_lat = accumulate(v.begin(), v.end(), 0.0) / v.size();

		// And we calculate perf
		perf_per_tid[tid].calc_perf(mean_lat);
	}
}

pid_t page_table_t::normalize_perf_and_get_worst_thread(){
	double min_p = 1e15;
	pid_t min_t = -1;

	int active_threads = 0;
	double mean_perf = 0.0;

	// We loop over TIDs
	for(auto const & t_it : tid_index) {
		pid_t tid = t_it.first;
		rm3d_data_t pd = perf_per_tid[tid];

		if(!pd.active)
			continue;

		active_threads++;

		// We sum the last_perf if active
		double pd_lp = pd.v_perfs[pd.index_last_node_calc];
		mean_perf += pd_lp;
		if(pd_lp < min_p){ // And we calculate the minimum
			min_t = tid;
			min_p = pd_lp;
		}
	}

	if(active_threads == 0) // Should never happen
		return -1;

	mean_perf /= active_threads;

	// We loop over TIDs again for normalising
	for(auto const & t_it : tid_index) {
		pid_t tid = t_it.first;
		rm3d_data_t* pd = &perf_per_tid[tid];

		if(!pd->active)
			continue;

		pd->v_perfs[pd->index_last_node_calc] /= mean_perf;
	}

	return min_t;
}

void page_table_t::reset_performance(){
	// We loop over TIDs
	for(auto const & t_it : tid_index) {
		pid_t tid = t_it.first;

		// And we call reset for them
		perf_per_tid[tid].reset();
	}
}

// Sum of last_performance for all active threads
double page_table_t::get_total_performance() {
	double val = 0.0;

	// We loop over TIDs
	for(auto const & t_it : tid_index) {
		pid_t tid = t_it.first;

		// And we sum them all if active
		rm3d_data_t pd = perf_per_tid[tid];
		if(!pd.active)
			continue;

		val += pd.v_perfs[pd.index_last_node_calc];
	}

	return val;
}

vector<double> page_table_t::get_perf_data(pid_t tid){
	return perf_per_tid[tid].v_perfs;
}

