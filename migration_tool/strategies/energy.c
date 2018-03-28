#include "energy.h"
#include "../rapl/energy_data.h"

vector<migration_cell_t> energy_str_t::get_pages_to_migrate(map<pid_t, page_table_t> *page_ts){
	const double MAX_THRES = 1.0; // Threshold maximum raw increase against base to do migrations
	const double DIFF_THRES = 0.75; // Threshold raw difference between highest and lowest to do migrations
	const size_t MAX_PAGES_TO_MIGRATE = 15;
	vector<migration_cell_t> v;

	// Regarding pages
	vector<double> rams = ed.get_curr_vals_from_domain("ram");

	// "ram" domain not supported, let's focus on threads
	if(rams.empty())
		return v;

	auto it_max = max_element(rams.begin(), rams.end());
	double mx = *(it_max);

	// No enough memory consumption increase -> no page migrations
	if(mx < MAX_THRES)
		return v;

	auto it_min = min_element(rams.begin(), rams.end());
	double mn = *(it_min);

	// If all nodes are quite memory loaded -> no page migrations
	if( (mx-mn) < DIFF_THRES)
		return v;

	// If we meet the criteria, we will migrate from the most loaded node to the least one
	size_t from = distance(rams.begin(), it_max);
	size_t to = distance(rams.begin(), it_min);

	// We search memory pages from "from" node to migrate
	for(auto const & t_it : *page_ts) { // Looping over PIDs
		if(v.size() == MAX_PAGES_TO_MIGRATE)
			break;

		pid_t pid = t_it.first;
		page_table_t table = t_it.second;

		for(auto const & p_it : table.page_node_map){ // Looping over pages
			if(v.size() == MAX_PAGES_TO_MIGRATE)
				break;

			long int addr = p_it.first;
			pg_perf_data_t perf = p_it.second;

			// Not from the desired node, to the next!
			if(perf.current_node != from)
				continue;

			// We won't migrate pages with "high" locality
			if(perf.acs_per_node[from] > 1)
				continue;

			migration_cell_t mc(addr, to, pid, false);
			v.push_back(mc);
		}
	}

	return v;
}

vector<migration_cell_t> energy_str_t::get_threads_to_migrate(map<pid_t, page_table_t> *page_ts){
	const double MAX_PERC_THRES = 0.3; // Threshold maximum increase ratio against base to do migrations
	const double DIFF_PER_THRES = 0.4; // Minimum ratio difference between lowest and highest to do migrations
	const size_t MAX_THREADS_TO_MIGRATE = system_struct_t::CPUS_PER_MEMORY / 2;
	vector<migration_cell_t> v;

	// Regarding threads
	vector<double> pkgs = ed.get_curr_vals_from_domain("pkg");

	auto it_max = max_element(pkgs.begin(), pkgs.end());
	double mx = *(it_max);
	size_t from = distance(pkgs.begin(), it_max);

	// If no enough pkg consumption increase -> no thread migrations
	double ratio = ed.get_ratio_against_base(mx, from, "pkg");
	if(ratio < MAX_PERC_THRES)
		return v;

	auto it_min = min_element(pkgs.begin(), pkgs.end());
	double mn = *(it_min);

	// If all nodes are quite memory loaded -> no thread migrations
	if( (mn/mx) < DIFF_PER_THRES)
		return v;

	// If we meet the criteria, we will migrate from the most loaded node to the least one
	size_t to = distance(pkgs.begin(), it_min);

	// We search TIDs from CPUs from "from" node to be migrated
	// [TODO]: use tid_cpu_table information to decide which threads to migrate
	set<int> picked_cpus;
	for(size_t c=0;c<system_struct_t::CPUS_PER_MEMORY && v.size() < MAX_THREADS_TO_MIGRATE;c++){
		int from_cpu = system_struct_t::node_cpu_map[from][c];

		for(pid_t const & tid : system_struct_t::cpu_tid_map[from_cpu]){
			if(v.size() == MAX_THREADS_TO_MIGRATE)
				break;

			// We search a, if possible, free CPU to migrate to
			int to_cpu = system_struct_t::get_free_cpu_from_node(to, picked_cpus);
			picked_cpus.insert(to_cpu);
			migration_cell_t mc(tid, to_cpu);
			v.push_back(mc);
		}
	}

	return v;
}

vector<migration_cell_t> energy_str_t::get_migrations(map<pid_t, page_table_t> *page_ts){
	vector<migration_cell_t> v1 = get_pages_to_migrate(page_ts);
	vector<migration_cell_t> v2 = get_threads_to_migrate(page_ts);

	v1.insert(v1.end(), v2.begin(), v2.end());
	return v1;
}
