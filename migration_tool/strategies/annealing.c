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
	int current_cell = system_struct_t::get_cpu_memory_cell(current_cpu);
	vector<double> current_perfs = page_t->get_perf_data(worst_tid);

	// Search potential core destinations from different memory nodes 
	for(int n=0; n<system_struct_t::NUM_OF_MEMORIES; n++){
		if(n == current_cell)
			continue;

		for(int const & actual_cpu : system_struct_t::node_cpu_map[n]){
			int tickets = get_tickets_from_perfs(n, current_cell, current_perfs, false);

			migration_cell mc(worst_tid, actual_cpu, current_cpu, page_t->pid, true); // Migration associated to this iteration (CPU)

			// Free core: posible simple migration with a determined score
			if(system_struct_t::is_cpu_free(actual_cpu)){
				tickets += TICKETS_FREE_CORE;

				labeled_migr_t lm(mc, tickets);
 				migration_list.push_back(lm);
				continue;
			}

			// Not a free core: get its TID info so a possible interchange can be planned
			pid_t other_tid = system_struct_t::get_tid_from_cpu(actual_cpu);
			vector<double> other_perfs = page_t->get_perf_data(other_tid);

			tickets += get_tickets_from_perfs(current_cell, n, other_perfs, true);
			
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
