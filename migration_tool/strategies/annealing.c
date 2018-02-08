#include "annealing.h"

//// Heavily based on what Óscar implemented in his PhD

double current_performance;
double last_performance = -1;
labeled_migr_t last_migration; // Always with 1 or 2 elements

/*** labeled_migr_t functions ***/
labeled_migr_t::labeled_migr(migration_cell_t mc, int t){
	potential_migr.push_back(mc);
	tickets = t;
}

labeled_migr_t::labeled_migr(migration_cell_t mc1, migration_cell_t mc2, int t){
	potential_migr.push_back(mc1);
	potential_migr.push_back(mc2);
	tickets = t;
}

bool labeled_migr_t::is_interchange() const {
	return potential_migr.size() == 2;
}

void labeled_migr_t::prepare_for_undo() {
	if(!is_interchange())
		potential_migr[0].interchange_dest();
	else {
		potential_migr[0].prev_dest = potential_migr[0].dest;
		potential_migr[1].prev_dest = potential_migr[1].dest;

		potential_migr[0].dest = potential_migr[1].prev_dest;
		potential_migr[1].dest = potential_migr[0].prev_dest;
	}
}

void labeled_migr_t::print() const {
	const char* const choices[] = {" simple migration", "n interchange"};
	printf("It is a%s. %d tickets.\n", choices[is_interchange()], tickets);
	for(migration_cell const & mc : potential_migr)
		mc.print();
}

/*** Rest of functions ***/
vector<migration_cell_t> undo_last_migration(){
	last_migration.prepare_for_undo();

	return last_migration.potential_migr;
}

// Gets a different number of tickets depending on perf values and if we are comparing a external thread for interchange or not (mod)
int get_tickets_from_perfs(int mem_cell, int current_cell, vector<double> perfs, bool mod){
	if(perfs[mem_cell] == PERFORMANCE_INVALID_VALUE)
		return TICKETS_MEM_CELL_NO_DATA[mod];
	else if(perfs[current_cell] > perfs[mem_cell])
		return TICKETS_MEM_CELL_WORSE[mod];
	else
		return TICKETS_MEM_CELL_BETTER[mod];
} 

vector<labeled_migr_t> get_candidate_list(pid_t worst_tid, page_table_t *page_t){
	vector<labeled_migr_t> migration_list;

	int current_cpu = system_struct_t::get_cpu_from_tid(worst_tid);
	int current_cell = system_struct_t::get_cpu_memory_node(current_cpu);
	vector<double> current_perfs = page_t->get_perf_data(worst_tid);

	// Search potential core destinations from different memory nodes 
	for(int n=0; n<system_struct_t::NUM_OF_MEMORIES; n++){
		if(n == current_cell)
			continue;

		for(size_t i=0;i<system_struct_t::CPUS_PER_MEMORY;i++){
			int actual_cpu = system_struct_t::node_cpu_map[n][i];

			int tickets = get_tickets_from_perfs(n, current_cell, current_perfs, false);

			migration_cell mc(worst_tid, actual_cpu, current_cpu, page_t->pid, true); // Migration associated to this iteration (CPU)

			// Free core: posible simple migration with a determined score
			if(system_struct_t::is_cpu_free(actual_cpu)){
				tickets += TICKETS_FREE_CORE;

				labeled_migr_t lm(mc, tickets);
 				migration_list.push_back(lm);
				continue;
			}

			// We will choose the TID with generates the higher number of tickets
			vector<pid_t> tids = system_struct_t::get_tids_from_cpu(actual_cpu);
			pid_t other_tid = -1;
			int aux_tickets = -1;

			for(pid_t const & aux_tid : tids){
				vector<double> other_perfs =  page_t->get_perf_data(aux_tid);
				int tid_tickets = get_tickets_from_perfs(current_cell, n, other_perfs, true);

				// Better TID to pick
				if(tid_tickets > aux_tickets){
					aux_tickets = tid_tickets;
					other_tid = aux_tid;
				}
			}

			tickets += aux_tickets;
			
			migration_cell mc2(other_tid, current_cpu, actual_cpu, page_t->pid, true);
			labeled_migr_t lm(mc, mc2, tickets);
			migration_list.push_back(lm);
		}
	}

	return migration_list;
}

// Picks migration or interchange from candidate list
labeled_migr_t get_random_labeled_cell(vector<labeled_migr_t> lm_list){
	int total_tickets = 0;
	for(labeled_migr_t const & lm : lm_list)
		total_tickets += lm.tickets;
	int result = rand() % total_tickets; // Gets random number up to total tickets

	for(labeled_migr_t const & lm : lm_list) {
		result -= lm.tickets; // Subtracts until lower than zero
		if(result < 0)
			return lm;
	}

	// Should never reach here
	return lm_list[0];
}

vector<migration_cell_t> get_iteration_migration(page_table_t *page_t){
	pid_t worst_tid = page_t->normalize_perf_and_get_worst_thread();

	#ifdef ANNEALING_PRINT
	printf("\n*\nWORST THREAD IS: %d\n", worst_tid);
	#endif

	//page_t->print_performance(); // Perfs after normalization
	
	// Selects migration targets for lottery (this is where the algoritm really is)
	vector<labeled_migr_t> migration_list = get_candidate_list(worst_tid, page_t);

	#ifdef ANNEALING_PRINT
	printf("\n*\nMIGRATION LIST CONTENT:");
	for(labeled_migr_t const & lm : migration_list)
		lm.print();
	#endif

	// I think this will only happen in no-NUMA systems, for testing
	if(migration_list.empty()){
		#ifdef ANNEALING_PRINT
		printf("TARGET LIST IS EMPTY, NO MIGRATIONS\n");
		#endif
		vector<migration_cell_t> emp;
		return emp;
	}

	labeled_migr_t target_cell = get_random_labeled_cell(migration_list);

	#ifdef ANNEALING_PRINT
	printf("\n*\nTARGET MIGRATION: ");
	target_cell.print();
	#endif

	// Saving for possible undo, prepared latter
	last_migration = target_cell;

	migration_list.clear();

	return target_cell.potential_migr;
}

// One PID version
vector<migration_cell_t> annealing_t::get_threads_to_migrate(page_table_t *page_t){
	vector<migration_cell_t> ret;

	current_performance = page_t->get_total_performance();
	double diff = current_performance / last_performance;

	#ifdef TH_MIGR_OUTPUT
	printf("\nCurrent Perf: %g. Last Perf: %g. Ratio: %g. SBM: %d\n",current_performance,last_performance,diff,get_time_value());
	#endif

	last_performance = current_performance;

	if(diff < 0) // First time or no data, so we do nothing
		last_performance = current_performance; // Redundant, but we need an useless sentence
	else if(diff < 0.9) { // We are doing MUCH worse, let's undo
		time_go_up();
		page_t->reset_performance();
		
		return undo_last_migration();
	} else if(diff < 1.0) // We are doing better or equal
		time_go_up();
	else // We are doing better or equal
		time_go_down();

	#ifdef ANNEALING_PRINT
	printf("\n*\nPERFORMING MIGRATION ALGORITHM\n");
	#endif

	ret = get_iteration_migration(page_t);

	// Cleans performance data and finishes iteration
	page_t->reset_performance();

	return ret;
}

/*** Functions for global version ***/
vector<labeled_migr_t> get_candidate_list(pid_t worst_tid, map<pid_t, page_table_t> *page_ts){
	vector<labeled_migr_t> migration_list;

	int current_cpu = system_struct_t::get_cpu_from_tid(worst_tid);
	int current_cell = system_struct_t::get_cpu_memory_node(current_cpu);

	pid_t current_pid = system_struct_t::get_pid_from_tid(worst_tid);
	vector<double> current_perfs = page_ts->operator[](current_pid).get_perf_data(worst_tid);

	// Search potential core destinations from different memory nodes 
	for(int n=0; n<system_struct_t::NUM_OF_MEMORIES; n++){
		if(n == current_cell)
			continue;

		for(size_t i=0;i<system_struct_t::CPUS_PER_MEMORY;i++){
			int actual_cpu = system_struct_t::node_cpu_map[n][i];
			int tickets = get_tickets_from_perfs(n, current_cell, current_perfs, false);

			migration_cell mc(worst_tid, actual_cpu, current_cpu, current_pid, true); // Migration associated to this iteration (CPU)

			// Free core: posible simple migration with a determined score
			if(system_struct_t::is_cpu_free(actual_cpu)){
				tickets += TICKETS_FREE_CORE;

				labeled_migr_t lm(mc, tickets);
 				migration_list.push_back(lm);
				continue;
			}

			// Not a free core: get its TIDs info so a possible interchange can be planned

			// We will choose the TID with generates the higher number of tickets
			vector<pid_t> tids = system_struct_t::get_tids_from_cpu(actual_cpu);
			pid_t other_tid = -1;
			int aux_tickets = -1;

			for(pid_t const & aux_tid : tids){
				pid_t other_pid = system_struct_t::get_pid_from_tid(aux_tid);
				vector<double> other_perfs = page_ts->operator[](other_pid).get_perf_data(aux_tid);
				int tid_tickets = get_tickets_from_perfs(current_cell, n, other_perfs, true);

				// Better TID to pick
				if(tid_tickets > aux_tickets){
					aux_tickets = tid_tickets;
					other_tid = aux_tid;
				}
			}

			tickets += aux_tickets;
			
			migration_cell mc2(other_tid, current_cpu, actual_cpu, system_struct_t::get_pid_from_tid(other_tid), true);
			labeled_migr_t lm(mc, mc2, tickets);
			migration_list.push_back(lm);
		}
	}

	return migration_list;
}


vector<migration_cell_t> get_iteration_migration(map<pid_t, page_table_t> *page_ts){
	pid_t worst_tid = -1;
	double min_p = 1e15;
	
	// normalize_perf_and_get_worst_thread() for all tables
	for(auto & t_it : *page_ts){
		page_table_t *t = &t_it.second;
		pid_t local_worst_t = t->normalize_perf_and_get_worst_thread();

		rm3d_data_t pd = t->perf_per_tid[local_worst_t];
		double local_worst_p = pd.v_perfs[pd.index_last_node_calc];

		if(local_worst_p < min_p){
			worst_tid = local_worst_t;
			min_p = local_worst_p;
		}
	}

	#ifdef ANNEALING_PRINT
	printf("\n*\nWORST THREAD IS: %d\n", worst_tid);
	#endif

	//for(auto& t_it : page_ts) // Perfs after normalization
		//t_it.second.print_performance();
	
	// Selects migration targets for lottery (this is where the algoritm really is)
	vector<labeled_migr_t> migration_list = get_candidate_list(worst_tid, page_ts);

	#ifdef ANNEALING_PRINT
	printf("\n*\nMIGRATION LIST CONTENT:");
	for(labeled_migr_t const & lm : migration_list)
		lm.print();
	#endif

	// I think this will only happen in no-NUMA systems, for testing
	if(migration_list.empty()){
		#ifdef ANNEALING_PRINT
		printf("TARGET LIST IS EMPTY, NO MIGRATIONS\n");
		#endif
		vector<migration_cell_t> emp;
		return emp;
	}

	labeled_migr_t target_cell = get_random_labeled_cell(migration_list);

	#ifdef ANNEALING_PRINT
	printf("\n*\nTARGET MIGRATION: ");
	target_cell.print();
	#endif

	// Saving for possible undo, prepared latter
	last_migration = target_cell;

	migration_list.clear();

	return target_cell.potential_migr;
}

vector<migration_cell_t> annealing_t::get_threads_to_migrate(map<pid_t, page_table_t> *page_ts){
	vector<migration_cell_t> ret;

	current_performance = 0.0;

	for(auto& t_it : *page_ts)
		current_performance += t_it.second.get_total_performance();
	double diff = current_performance / last_performance;

	#ifdef TH_MIGR_OUTPUT
	printf("\nCurrent Perf: %g. Last Perf: %g. Ratio: %g. SBM: %d\n",current_performance,last_performance,diff,get_time_value());
	#endif

	last_performance = current_performance;

	if(diff < 0) // First time or no data, so we do nothing
		last_performance = current_performance; // Redundant, but we need an useless sentence
	else if(diff < 0.9) { // We are doing MUCH worse, let's undo
		time_go_up();

		for(auto& t_it : *page_ts)
			t_it.second.reset_performance();
		
		return undo_last_migration();
	} else if(diff < 1.0) // We are doing better or equal
		time_go_up();
	else // We are doing better or equal
		time_go_down();

	#ifdef ANNEALING_PRINT
	printf("\n*\nPERFORMING MIGRATION ALGORITHM\n");
	#endif

	ret = get_iteration_migration(page_ts);

	// Cleans performance data and finishes iteration
	for(auto& t_it : *page_ts)
		t_it.second.reset_performance();

	return ret;

}
