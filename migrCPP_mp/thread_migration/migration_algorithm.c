#include "migration_algorithm.h"						

int total_thread_migrations = 0;

int reset_step;

//FOR TIME RESET
double last_performance=-1;
double current_performance=1;

tid_cell_t * last_migration_thread_cell_1=NULL;
tid_cell_t * last_migration_thread_cell_2=NULL;
int last_migration_core=-1;
char already=0;


int migration_worst_to_other_mem_cell_target_weighted_by_past_performance(pid_t pid, pid_list_t* pid_m);

int init_migration_algorithm(){
	reset_step=SYS_RESET_STEP;
	init_aux_functions();
}

int migrateThreads(pid_t pid, pid_list_t* pid_m, unsigned int step){
	int ret=-1;

	if(pid_m->is_empty()){
		printf("PID list empty\n");
		return ret;
	}

	set_migration_step(step);
	reset_step--;

	/*
	 * For more than 1 process
	*/

	/*
	 * CHECK IF THREADS ARE PINNED
	 * it was done inside the ordering procedures, needs to be here for everything else
	*/
	//pid_m->print_pid_list_redux();
	pin_all_threads_with_pid_greater_than_to_free_cores_and_move_and_free_cores_if_inactive(pid_m, pid);
	//printf("**\n**\n");
	//printf("Printing PID list...\n");
	//pid_m->print();


	/*
	 * CHECK IF LAST MIGRATION WAS BETTER OR WORSE
	*/

	// Code to debug performance calculation
/*
	int c = 0;
	tid_l tids = *(pid_m->get_tid_list(pid));
	for(int i=0;i<tids.size();i++){
		pid_t tid = tids[i];
		tid_cell_t* t = pid_m->get_tid_cell(tid);
		printf("%d %d %d, ", pid, t->tid, t->active);
		if(t->active){
			c++;
			printf("%.2f, ", t->performance.last_DyRM_performance);
		}
	}
	printf("tid activos con pid valido: %d\n", c);
*/
	current_performance = pid_m->calculate_total_last_performance_from_pid(pid);
	double diff = current_performance/last_performance;

	#ifdef THREAD_MIGRATION_OUTPUT
		printf("\nCurrent Perf %g, Last Perf %g, diff %g, SBM %d\n",current_performance,last_performance,diff,get_time_value());
	#endif

	if(diff<0){
		//nothing, first time
		last_performance=current_performance;
	}else if(diff<0.9){//we are doing MUCH worse, go back
		time_go_up();
		ret=undo_last_migration();
		last_performance=current_performance;
		//go to Clean up
		pid_m->tid_m.set_inactive();
		return ret;
	}else if(diff<1){//we are doing better or equal
		time_go_up();
	}else{//we are doing better or equal
		time_go_down();
	}
	last_performance=current_performance;
	/*
	 * PERFORM MIGRATION
	*/
	//printf("\n*\nPERFORMING MIGRATION ALGORITYHM\n");
	 ret=migration_worst_to_other_mem_cell_target_weighted_by_past_performance(pid, pid_m);

	/*
	 * CLEAN UP AND GET READY FOR NEXT
	*/
	pid_m->tid_m.set_inactive();

	return ret;
}

//badly done
int undo_last_migration(){
	int ret;
	if(already){
		#ifdef THREAD_MIGRATION_OUTPUT
			printf("error undoing migration, undoing already done\n");
		#endif
		return -2;
	}
	if(last_migration_thread_cell_1==NULL){
		#ifdef THREAD_MIGRATION_OUTPUT
			printf("error undoing migration\n");
		#endif
		return -1;
	}
	if(last_migration_thread_cell_2==NULL){
		if(last_migration_core==-1){
			#ifdef THREAD_MIGRATION_OUTPUT
				printf("error undoing migration\n");
			#endif
			return -1;
		}else{
			#ifdef THREAD_MIGRATION_OUTPUT
				printf("Undoing last migration\n");
			#endif
			total_thread_migrations++;
			ret=migrate_pinned_thread_and_free_table(last_migration_thread_cell_1, last_migration_core);
			return ret;
		}
	}else{
		#ifdef THREAD_MIGRATION_OUTPUT
			printf("Undoing last migration\n");
		#endif
		total_thread_migrations++;
		ret=interchange_pinned_threads(last_migration_thread_cell_1, last_migration_thread_cell_2);
		return ret;
	}
	return -1;
}


int migration_worst_to_other_mem_cell_target_weighted_by_past_performance(pid_t pid, pid_list_t* pid_m){
	int ret=0;
	/*
	 * NORMALISE PERFORMANCE TO COMPARE AND CHOOSE WORST THREAD
	*/
	//pid_m->print();
	tid_cell_t* worst_thread_cell = pid_m->normalise_all_last_DyRM_to_mean_by_pid_and_return_worst(pid);

	if(worst_thread_cell==NULL){
		#ifdef THREAD_MIGRATION_OUTPUT
			printf("COULD NOT FIND WORST THREAD\n");
		#endif
		return -1;
	}

	//printf("\n***\n");
	//pid_m->print();
	//printf("\n*\nWORST THREAD IS\n");
	//worst_thread_cell->print();
	
	/*
	 * SELECT MIGRATION TARGETS FOR LOTTERY (This is where the algoritm really is)
	*/
	migration_list_t migration_list = migration_destinations_wweight(worst_thread_cell, pid_m);
	//printf("\n*\n");
	//migration_list.print();

	/*
	 * GET MIGRATION TARGET FROM LOTTERY
	*/
	migration_cell_t* target_migration_cell = NULL;
	if(migration_list.is_empty()){
		#ifdef THREAD_MIGRATION_OUTPUT
			printf("TARGET LIST IS EMPTY, NO MIGRATIONS\n");
		#endif
		return 0;
	} else {
		target_migration_cell = migration_list.get_random_weighted_migration_cell();
		//printf("\n*\nTARGET MIGRATION IS\n");
		//target_migration_cell.print();
		/*
		 * PERFORM MIGRATION
		*/
		//printf("\n*\nMIGRATING\n");
		total_thread_migrations++;
		migrate(worst_thread_cell, target_migration_cell);
	}

	/*
	 * SAVE FOR UNDO
	*/
	last_migration_thread_cell_1=worst_thread_cell;
	if(target_migration_cell != NULL){
		last_migration_thread_cell_2=target_migration_cell->tid_cell;
		last_migration_core=target_migration_cell->free_core;
	}

	/*
	 * CLEAN UP AND GET READY FOR NEXT
	*/
	migration_list.clear();
	return ret;
}

/*
 * There was a lot of code here in previous versions, see migration_tool_v6
*/

