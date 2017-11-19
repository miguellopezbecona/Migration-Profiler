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

