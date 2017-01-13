#include "thread_migration_aux.h"

/**
 ** PRIVATE FUNCTIONS
**/

int migrate_pinned_thread_and_free_table(tid_cell_t *tid_c, int target_core);
int find_first_free_core();


//SYS_MEM_STRUCT

unsigned int migration_step;
unsigned int best_step;


void init_aux_functions(){
	migration_step=0;
	init_migration_table();
}

//This is old, performance is more complex
void print_target_thread(target_thread_t target){
	printf("TID: %d, LATENCY: %g, CORE: %u, TARGET: %u\n",target.tid_c->tid,target.tid_c->performance.mean_latency[0], target.tid_c->last_core, target.target_core);
}

void set_affinity_error(){
	switch(errno){
		case EFAULT:
			printf("Error setting affinity: A supplied memory address was invalid\n");
			break;
		case EINVAL:
			printf("Error setting affinity: The affinity bitmask mask contains no processors that are physically on the system, or cpusetsize is smaller than the size of the affinity mask used by the kernel\n");
			break;
		case EPERM:
			printf("Error setting affinity: The calling process does not have appropriate privileges\n");
			break;
		case ESRCH:
			printf("Error setting affinity: The process whose ID is pid could not be found\n");
			break;
	}
}

unsigned int get_migration_step(){
	return migration_step;
}

void set_migration_step(unsigned int step){
	migration_step=step;
}


int remove_core_all_nofixed_tids(int core, tid_list_t* tid_l){
	tid_cell_t* tid_c;

	for(tr1::unordered_map<pid_t, tid_cell_t>::iterator it = tid_l->map.begin(); it != tid_l->map.end(); ++it) {
		tid_c = &it->second;
		if(!tid_c->pinned){
			CPU_CLR(core,&(tid_c->affinity));
		}
	}

	return 0;
}

int add_core_all_nofixed_tids(int core, tid_list_t* tid_l){
	tid_cell_t* tid_c;

	for(tr1::unordered_map<pid_t, tid_cell_t>::iterator it = tid_l->map.begin(); it != tid_l->map.end(); ++it) {
		tid_c = &it->second;
		if(!tid_c->pinned){
			CPU_SET(core,&(tid_c->affinity));
		}
	}
	return 0;
}

int pin_tid_to_core(int core, tid_cell_t *tid_c){
	CPU_ZERO(&(tid_c->affinity));
	CPU_SET(core,&(tid_c->affinity));
	tid_c->pinned = true;
	tid_c->assigned_core = core;
	return 0;
}

int unpin_tid(tid_cell_t *tid_c){
	sched_getaffinity(0, sizeof(cpu_set_t), &(tid_c->affinity));
	tid_c->pinned = false;
	tid_c->assigned_core = -1;
	return 0;
}


int update_affinity_tid(tid_cell_t *tid_c){
	//printf("%d:CPU_COUNT() of set:    %d\n",tid_c->tid, CPU_COUNT(&(tid_c->affinity)));
	if(sched_setaffinity(tid_c->tid,sizeof(cpu_set_t),&(tid_c->affinity))){
		set_affinity_error();
	}

	tid_c->step_last_migrated=migration_step;
	return 0;
}

int update_affinities_all_tids(tid_list_t* tid_l){
	tid_cell_t* tid_c;

	for(tr1::unordered_map<pid_t, tid_cell_t>::iterator it = tid_l->map.begin(); it != tid_l->map.end(); ++it) {
		tid_c = &it->second;
		//printf("%d:CPU_COUNT() of set:    %d\n",tid_c->tid, CPU_COUNT(&(tid_c->affinity)));
		update_affinity_tid(tid_c);
	}
	return 0;
}


int pin_all_threads(tid_list_t* tid_l){
	tid_cell_t* tid_c;

	for(tr1::unordered_map<pid_t, tid_cell_t>::iterator it = tid_l->map.begin(); it != tid_l->map.end(); ++it) {
		tid_c = &it->second;
		if(!tid_c->pinned){
			pin_tid_to_core_or_first_free_core(tid_c->last_core,tid_c);
		}
	}
	return 0;
}

//check active!!
int pin_all_threads_to_free_cores_and_move(tid_list_t* tid_l){
	tid_cell_t* tid_c;

	for(tr1::unordered_map<pid_t, tid_cell_t>::iterator it = tid_l->map.begin(); it != tid_l->map.end(); ++it) {
		tid_c = &it->second;
		if(!tid_c->pinned){
			pin_tid_to_core_or_first_free_core(tid_c->last_core,tid_c);
			update_affinity_tid(tid_c);
		}
	}
	return 0;
}

int pin_thread_to_free_core_and_move_free_cores_if_inactive(tid_cell_t* tid_c){
	if(!tid_c->pinned && tid_c->active){
		pin_tid_to_core_or_first_free_core(tid_c->last_core,tid_c);
		update_affinity_tid(tid_c);
	}else if(!tid_c->active){ //if tid is pinned, check if active
		free_table_core(tid_c->assigned_core, tid_c->tid);
		unpin_tid(tid_c);
		//update_affinity_tid(tid_c);
	}

	return 0;
}

// "Greater" should be "equal"
int pin_all_threads_with_pid_greater_than_to_free_cores_and_move_and_free_cores_if_inactive(pid_list_t* pid_l, pid_t pid){
	tid_l* tids = pid_l->get_tid_list(pid); // Gets TIDs associated to PID
	for(int i=0;i<tids->size();i++){
		pid_t tid = (*tids)[i];
		tid_cell_t* cell = pid_l->get_tid_cell(tid); // Gets tid_cell_t associated to TID
		pin_thread_to_free_core_and_move_free_cores_if_inactive(cell);
	}
	
	return 0;
}

int migrate_pinned_thread_and_free_table(tid_cell_t *tid_c, int target_core){
	free_table_core(tid_c->assigned_core, tid_c->tid);
	pin_tid_to_core(target_core, tid_c);
	set_table_core(target_core, tid_c->tid);
	update_affinity_tid(tid_c);
	#ifdef THREAD_MIGRATION_OUTPUT
		printf("MIGRATED: TID-%d to %d\n",tid_c->tid,target_core);
	#endif
	return target_core;
}

int interchange_pinned_threads(tid_cell_t *tid1_c, tid_cell_t *tid2_c){
	if((tid1_c->assigned_core==-1)||(tid2_c->assigned_core==-1)){
		printf("tids are not pinned, can not interchange\n");
		return -1;
	}
	int aux_core = tid2_c->assigned_core;//current core of interchanging thread

	//move the current occupant of the core to a new one
	pin_tid_to_core(tid1_c->assigned_core, tid2_c);
	free_table_core(tid1_c->assigned_core, tid1_c->tid); //free its spot
	set_table_core(tid1_c->assigned_core, tid2_c->tid);

	//move the target to the new core
	pin_tid_to_core(aux_core, tid1_c);
	free_table_core(aux_core, tid2_c->tid); //free its spot
	set_table_core(aux_core, tid1_c->tid);
	update_affinity_tid(tid1_c);
	update_affinity_tid(tid2_c);
	#ifdef THREAD_MIGRATION_OUTPUT
		printf("INTERCHANGED: TID-%d to %d and TID-%d to %d\n",tid1_c->tid,tid1_c->assigned_core,tid2_c->tid,tid2_c->assigned_core);
	#endif
	return 0;
}
	

//returns -2 if error, -1 if not free core, core number on success
int migrate_thread_to_free_core(tid_cell_t *tid_c){
	int core = find_first_free_core();
	if(core>-1){
		printf("Free core in %d\n",core);
		core=migrate_pinned_thread_and_free_table(tid_c, core);
	}
	return core;
}

//returns -2 if error, -1 if not free core, core number on success
int migrate_thread_to_random_free_core(tid_cell_t *tid_c){
	int core = get_random_free_core();
	if(core>-1){
		printf("Random free core in %d\n",core);
		core=migrate_pinned_thread_and_free_table(tid_c, core);
	}
	return core;
}


int migrate(tid_cell_t *tid_c, migration_cell_t *target_migration){
	int ret;
	if(target_migration->free_core > -1){//It is a free core
		ret=migrate_pinned_thread_and_free_table(tid_c, target_migration->free_core);
	}else{//It is another thread
		ret=interchange_pinned_threads(tid_c, target_migration->tid_cell);
	}
	return ret;
}

//current_mem_cell is wanted to avoid having to recalculate
int get_lottery_weight_for_mem_cell(int mem_cell,int current_cell, tid_cell_t *thread_cell){
	int tickets;
	if(mem_cell==current_cell){
		return TICKETS_MEM_CELL_NO_VALID;
	}else if(thread_cell->performance.DyRM_performance[mem_cell]==PERFORMANCE_INVALID_VALUE){
		return TICKETS_MEM_CELL_NO_DATA;
	}else if(thread_cell->performance.DyRM_performance[current_cell] > thread_cell->performance.DyRM_performance[mem_cell]){
		return TICKETS_MEM_CELL_WORSE;
	}else{
		return TICKETS_MEM_CELL_BETTER;
	}
} 
//This uses the second values, it is the same as before, only used if different values for the twio threads are wanted
int get_lottery_weight_for_mem_cell_2(int mem_cell,int current_cell, tid_cell_t *thread_cell){
	int tickets;
	if(mem_cell==current_cell){
		return TICKETS_MEM_CELL_NO_VALID;
	}else if(thread_cell->performance.DyRM_performance[mem_cell]==PERFORMANCE_INVALID_VALUE){
		return TICKETS_MEM_CELL_NO_DATA_2;
	}else if(thread_cell->performance.DyRM_performance[current_cell] > thread_cell->performance.DyRM_performance[mem_cell]){
		return TICKETS_MEM_CELL_WORSE_2;
	}else{
		return TICKETS_MEM_CELL_BETTER_2;
	}
} 


//uses only migration table
migration_list_t migration_destinations_wweight(tid_cell_t *target_thread, pid_list_t* pid_m){
	int ret = 0;
	int current_cell = get_cpu_memory_cell(target_thread->assigned_core);
	int destiny_cell;
	int tickets, extra_tickets;
	pid_t tid;
	tid_cell_t *t_cell=NULL;
	migration_list_t migration_list;

	for(int i=0;i<SYS_NUM_OF_CORES;i++){
		tickets=0;
		destiny_cell=get_cpu_memory_cell(i);
		tickets = get_lottery_weight_for_mem_cell(destiny_cell, current_cell, target_thread);

		//check if it is on a different memory cell
		if(tickets!=TICKETS_MEM_CELL_NO_VALID){
			if(is_core_free(i)){//this is here for compatibility with multiple threads per core, not really necessary with one, because it could be done in next step
				tickets = tickets + TICKETS_FREE_CORE;
				ret = migration_list.add_cell(NULL,i, tickets);
			}else{
				//for every possible thread in the core (does not make much sense in this algorithm, but compatibility)
				for(int j=0;j<MAX_THREADS_PER_CORE;j++){
					tid = get_tid_in_core(i, j);
					t_cell = pid_m->get_tid_cell(tid);

					tickets = tickets + get_lottery_weight_for_mem_cell_2(current_cell, destiny_cell, t_cell);
					if(tickets>0){
						migration_list.add_cell(t_cell,-1, tickets);
					}
				}
			}
		}
	}

	return migration_list;
}
	
