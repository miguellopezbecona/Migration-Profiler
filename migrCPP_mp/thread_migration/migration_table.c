#include "migration_table.h"

int core_table_t::init(){
	for(int i=0;i<SYS_NUM_OF_CORES;i++)
		list[i] = -1;
}

void core_table_t::print(){
	printf("THREAD ASSIGMENT PER CORE TABLE \n");
	for(int i=0;i<SYS_NUM_OF_CORES;i++){
		printf("CORE: %d -> ", i);

		if(list[i] < 0)
			printf("UNASSIGNED");
		else
			printf("TID %d",list[i]);
		
		printf("\n");
	}
}

int core_table_t::free_tid(int core, pid_t tid){
	if(list[core] == tid){
		list[core] = -1;
		//tid_c->pinned=0;
	}
	return 0;
}

int core_table_t::set_tid_to_core(int core, pid_t tid){
	if(list[core] < 0)
		list[core] = tid;

	return 0;
}

//returns -1 if not free core
int core_table_t::find_first_free_core(){
	for(int i=0;i<SYS_NUM_OF_CORES;i++){
		if(list[i] < 0)
			return i;
	}
	return -1; //no free cores
}

int core_table_t::find_next_free_core(int core){
	for(int i=core;i<SYS_NUM_OF_CORES;i++){
		if(list[i] < 0)
			return i;
	}
	return -1; //no free cores
}


//INNEFICIENT!!!!!!!
int core_table_t::get_random_free_core(){
	int ret, counter;
	int core = find_first_free_core();
	if(core==-1){//No free cores
		return -1;
	}
	counter = rand()%SYS_NUM_OF_CORES;//should be number of free cores for real random, change later
	while(counter){
		ret=find_next_free_core(core);
		if(ret==-1){//no more free cores
			return core;//last free core;
		}
		core=ret;
		counter--;
	}
	return core;
}


int core_table_t::pin_tid_to_core_and_set_table(int core, tid_cell_t *tid_c){
	CPU_ZERO(&(tid_c->affinity));
	CPU_SET(core,&(tid_c->affinity));
	tid_c->pinned=1;
	tid_c->assigned_core = core;
	set_tid_to_core(core, tid_c->tid);
	return 0;
}

//returns -1 if no free spots, can be MAX_THREADS_PER_CORE
int core_table_t::pin_tid_to_first_free_core(tid_cell_t *tid_c){
	for(int i=0;i<SYS_NUM_OF_CORES;i++){
		if(list[i] < 0){
			pin_tid_to_core_and_set_table(i,tid_c);
			return i;
		}
	}

	return -1;//no free cores
}

//returns -1 if no free spots, can be MAX_THREADS_PER_CORE
int core_table_t::pin_tid_to_core_or_first_free_core(int core, tid_cell_t *tid_c){
	if(list[core] < 0){
		pin_tid_to_core_and_set_table(core,tid_c);
		return core;
	} else
		return pin_tid_to_first_free_core(tid_c); //preferred core not free
	
}

bool core_table_t::is_core_free(int core){
	return list[core] < 0;
}

pid_t core_table_t::get_tid_in_core(int core){
	//sanity check, disabled
	/*
	if(core > (SYS_NUM_OF_CORES - 1) )
		core = 0;
	*/
	return list[core];
}

