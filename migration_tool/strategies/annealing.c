#include "annealing.h"

//// Heavily based on what Óscar implemented in his PhD

double current_performance;
double last_performance = -1;
vector<migration_cell_t> last_migrations; // Always with 1 or 2 elements
int last_migration_core;

vector<migration_cell_t> undo_last_migration(){
	size_t sz = last_migrations.size();
	vector<migration_cell_t> ret;

	switch(sz){
		case 0: // No last migration
			printf("Error undoing migration\n");
			break;
		case 1:
			printf("Undoing last migration.\n");
			last_migrations[0].dest = last_migration_core;
			ret.push_back( last_migrations[0] );
			break;
		case 2:
			printf("Undoing last interchange.\n");
			ret.push_back( last_migrations[0] );
			ret.push_back( last_migrations[1] );
			break;
	}

	return ret;
}

//current_mem_cell is wanted to avoid having to recalculate
int get_lottery_weight_for_mem_cell(int mem_cell, int current_cell, vector<double> perfs){
	if(perfs[mem_cell] == PERFORMANCE_INVALID_VALUE)
		return TICKETS_MEM_CELL_NO_DATA;
	else if(perfs[current_cell] > perfs[mem_cell])
		return TICKETS_MEM_CELL_WORSE;
	else
		return TICKETS_MEM_CELL_BETTER;
} 

//This uses the second values, it is the same as before, only used if different values for the two threads are wanted
int get_lottery_weight_for_mem_cell_2(int mem_cell, int current_cell, vector<double> perfs){
	if(perfs[mem_cell] == PERFORMANCE_INVALID_VALUE)
		return TICKETS_MEM_CELL_NO_DATA_2;
	else if(perfs[current_cell] > perfs[mem_cell])
		return TICKETS_MEM_CELL_WORSE_2;
	else
		return TICKETS_MEM_CELL_BETTER_2;
} 

vector<migration_cell_t> migration_destinations_wweight(pid_t worst_tid, page_table_t *page_t){
	vector<migration_cell_t> migration_list;

	int current_cpu = system_struct_t::get_cpu_from_tid(worst_tid);
	int current_cell = system_struct_t::get_cpu_memory_cell(current_cpu);
	vector<double> current_perfs = page_t->get_perf_data(worst_tid);

	// Search potential core destinations from different memory nodes 
	for(int n=0; n<system_struct_t::NUM_OF_MEMORIES; n++){
		if(n == current_cell)
			continue;

		for(int c=0; c<system_struct_t::CORES_PER_MEMORY; c++){
			int actual_core = system_struct_t::get_ordered_cpu_from_node(n, c);
			int tickets = get_lottery_weight_for_mem_cell(n, current_cell, current_perfs);

			// Free core: posible simple migration with a determined score
			if(system_struct_t::is_cpu_free(current_cpu)){
				tickets += TICKETS_FREE_CORE;

				migration_cell mc(worst_tid, n, page_t->pid, true);
 				migration_list.push_back(mc); // How to take "tickets" into account?
				continue;
			}

			// Not a free core: get its TID info so a possible interchange can be planned
			pid_t other_tid = system_struct_t::get_tid_from_cpu(current_cpu);
			vector<double> other_perfs = page_t->get_perf_data(other_tid);

			tickets += get_lottery_weight_for_mem_cell_2(current_cell, n, other_perfs);
			
			// How to take "tickets" and interchange into account?
			migration_cell mc1(worst_tid, n, page_t->pid, true);
			migration_cell mc2(other_tid, current_cell, page_t->pid, true);
			migration_list.push_back(mc1);
			migration_list.push_back(mc2);
		}
	}

	return migration_list;
}


vector<migration_cell_t> migrate_worst_thread(page_table_t *page_t){
	vector<migration_cell_t> ret;

	// [TODO]: normalize everything to worst performance

	pid_t worst_tid = page_t->get_worst_thread();

	//printf("\n*\nWORST THREAD IS: %d\n", worst_tid);
	
	// SELECT MIGRATION TARGETS FOR LOTTERY (This is where the algoritm really is)
	vector<migration_cell> migration_list = migration_destinations_wweight(worst_tid, page_t);

	//printf("\n*\n");
	//migration_list.print();

	// GET MIGRATION TARGET FROM LOTTERY
/*
	migration_cell_t* target_migration_cell = NULL;
	if(migration_list.is_empty()){
		printf("TARGET LIST IS EMPTY, NO MIGRATIONS\n");
		return 0;
	}else{
		target_migration_cell = migration_list.get_random_weighted_migration_cell();
		//printf("\n*\nTARGET MIGRATION IS\n");
		//target_migration_cell.print();
		// PERFORM MIGRATION
		//printf("\n*\nMIGRATING\n");
		migrate(worst_thread_cell, target_migration_cell);
	}

	// SAVE FOR UNDO
	last_migration_thread_cell_1=worst_thread_cell;
	if(target_migration_cell != NULL){
		last_migration_thread_cell_2=target_migration_cell->tid_cell;
		last_migration_core=target_migration_cell->free_core;
	}
*/

	// CLEAN UP AND GET READY FOR NEXT
	migration_list.clear();

	return ret;
}

vector<migration_cell_t> annealing_t::get_threads_to_migrate(page_table_t *page_t){
	vector<migration_cell_t> ret;

	//pin_all_threads_with_pid_greater_than_to_free_cores_and_move_and_free_cores_if_inactive(pid_l, pid);

	current_performance = page_t->get_total_performance();
	double diff = current_performance / last_performance;
	printf("\nCurrent Perf %g, Last Perf %g, diff %g, SBM %d\n",current_performance,last_performance,diff,get_time_value());

	if(diff < 0) // nothing, first time
		last_performance = current_performance;
	else if(diff < 0.9) { //we are doing MUCH worse, go back
		time_go_up();
		ret = undo_last_migration();
		last_performance = current_performance;
		
		//go to Clean up
		page_t->reset_performance();
		return ret;
	} else if(diff < 1.0) // we are doing better or equal
		time_go_up();
	else // we are doing better or equal
		time_go_down();

	last_performance = current_performance;

	/// PERFORM MIGRATION
	//printf("\n*\nPERFORMING MIGRATION ALGORITYHM\n");
	ret = migrate_worst_thread(page_t);

	// CLEAN UP AND GET READY FOR NEXT
	page_t->reset_performance();

	return ret;
}
