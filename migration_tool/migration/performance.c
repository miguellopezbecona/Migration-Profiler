#include "performance.h"

/*** *_perf_data_t functions ***/
void base_perf_data_t::print() const {
	if(num_acs_thres > 0)
		printf("UNIQ_ACS: %d, ACS_THRES: %d, MIN_LAT: %d, MEDIAN_LAT: %d, MAX_LAT: %d\n", num_uniq_accesses, num_acs_thres, min_latency, median_latency, max_latency);
}

void pg_perf_data_t::print() const {
	if(num_acs_thres > 0)
		printf("UNIQ_ACS: %d, ACS_THRES: %d, MIN_LAT: %d, MEDIAN_LAT: %d, MAX_LAT: %d\n", num_uniq_accesses, num_acs_thres, min_latency, median_latency, max_latency);

	printf("MEM_NODE: %d, ACS_PER_NODE: {", current_node);
	for(size_t i=0;i<system_struct_t::NUM_OF_MEMORIES;i++)
		printf(" %d", acs_per_node[i]);
	printf(" }\n");
}


/*** rm3d_data_t functions ***/
void rm3d_data_t::add_data(int cpu, long int inst, long int req, long int time){
	active = true;

	insts[cpu] += inst;
	reqs[cpu] += req;
	times[cpu] += time;
}

// Like Óscar did, after the sums for each CPU, the main formula is applied for each one and then stored in its mem node
void rm3d_data_t::calc_perf(double mean_lat){
	double inv_mean_lat = 1 / mean_lat;

	for(int cpu = 0; cpu < system_struct_t::NUM_OF_CPUS; cpu++) {
		if(reqs[cpu] == 0) // No data, bye
			continue;

		// Divide by zeros check should be made. It's assumed it won't happen if the thread is inactive
		double inst_per_s = (double) insts[cpu] * 1000 / times[cpu]; // Óscar used inst/ms, so * 10^3
		double inst_per_b = (double) insts[cpu] / (reqs[cpu] * CACHE_LINE_SIZE);

		int cpu_node = system_struct_t::get_cpu_memory_cell(cpu);
		v_perfs[cpu_node] = inv_mean_lat * inst_per_s * inst_per_b;

		// Debug
		//printf("PERF: %.2f, MEAN_LAT = %.2f, INST_S = %.2f, INST_B = %.2f\n", v_perfs[cpu_node], mean_lat, inst_per_s, inst_per_b);

		index_last_node_calc = cpu_node;
	}
}

void rm3d_data_t::reset() {
	active = false;

	fill(insts.begin(), insts.end(), 0);
	fill(reqs.begin(), reqs.end(), 0);
	fill(times.begin(), times.end(), 0);
	fill(v_perfs.begin(), v_perfs.end(), PERFORMANCE_INVALID_VALUE);
}

void rm3d_data_t::print() const {
	for(int cpu = 0; cpu < system_struct_t::NUM_OF_CPUS; cpu++)
		printf("\tCPU: %d, INSTS = %lu, REQS = %lu, TIMES = %lu\n", cpu, insts[cpu], reqs[cpu], times[cpu]);

	for(int node = 0; node < system_struct_t::NUM_OF_MEMORIES; node++)
		printf("\tNODE: %d, PERF = %.2f\n", node, v_perfs[node]);
}


/*** Experimental structures code ***/
perf_table_t tid_cpu_table; // system_struct_t::NUM_OF_CPUS is not passed because it's not calculated in this point

/** perf_cell **/
perf_cell_t::perf_cell(){
	num_acs = 0;
	mean_lat = 0.0;
}

perf_cell_t::perf_cell(double lat){
	num_acs = 1;
	mean_lat = lat;
}

void perf_cell_t::update_mlat(double lat){
	mean_lat = (mean_lat*num_acs + lat) / (num_acs + 1);
	num_acs++;
}

bool perf_cell_t::is_filled() const {
	return num_acs > 0;
}

void perf_cell_t::print() const {
	if(!is_filled())
		printf("Not filled.\n");
	else
		printf("NUM_ACS: %d, MEAN_LAT: %.2f\n", num_acs, mean_lat);
}

/** perf_table **/
double perf_table_t::alfa = 1.0;

perf_table_t::perf_table(){
	coln = 1;
}

perf_table_t::perf_table(unsigned short n){
	coln = n;
}

perf_table_t::~perf_table(){
	// Frees pointer for each row
	for(auto const & it : table)
		free(it.second);

	table.clear();
}

bool perf_table_t::has_key(long int key){
	return table.count(key) > 0;
}

void perf_table_t::remove_key(long int key){
	table.erase(key);
}

// When we define a sole performance metric, we should apply an aging technique
void perf_table_t::add_data(long int key, int col_num, double lat){
	if(has_key(key)) // Entry already exists, just updates mean latency
		table[key][col_num].update_mlat(lat);
	else { // No entry. We create it and we enter initial latency in the selected CPU
		table[key] = (perf_cell_t*)malloc(coln*sizeof(perf_cell_t));
		
		for(int i=0;i<coln;i++){
			perf_cell_t pc;
			table[key][i] = pc;
		}

		table[key][col_num].update_mlat(lat);	
	}
}

void perf_table_t::print() const {
	const char* const keys[] = {"TID", "Page address"};
	const char* const colns[] = {"CPU", "Node"};
	bool is_page_table = (coln == system_struct_t::NUM_OF_MEMORIES);

	printf("Printing perf table (%s/%s type)\n", keys[is_page_table], colns[is_page_table]);

	// Prints each row
	for(auto const & it : table) {
		pid_t tid = it.first;
		perf_cell_t *pc = it.second;

		printf("%s: %d\n", keys[is_page_table], tid);
		
		// Prints each cell
		for(int i=0;i<coln;i++){
			printf("\t%s %d: ", colns[is_page_table], i);
			pc[i].print();
		}
	}

	printf("\n");
}
