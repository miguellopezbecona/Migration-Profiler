#include "migration_table.h"

//to save which tid are set to which core (limit of two for now)
pid_t core_table[SYS_NUM_OF_CORES][MAX_THREADS_PER_CORE];

int init_migration_table(){
	for(int i=0;i<SYS_NUM_OF_CORES;i++){
		for(int j=0;j<MAX_THREADS_PER_CORE;j++)
			core_table[i][j] = -1;
	}
}


void print_core_table(){
	printf("THREAD ASSIGMENT PER CORE TABLE \n");
	for(int i=0;i<SYS_NUM_OF_CORES;i++){
		printf("CORE: %d",i);
		for(int j=0;j<MAX_THREADS_PER_CORE;j++){
			printf(", ");
			if(core_table[i][j] < 0){
				printf("[%d] UNASSIGNED",j);
			}else{
				printf("TID %d",core_table[i][j]);
			}
		}
		printf("\n");
	}
}


int free_table_core(int core, pid_t tid){
	for(int i=0;i<MAX_THREADS_PER_CORE;i++){
		if(core_table[core][i] == tid){
			core_table[core][i] = -1;
			//tid_c->pinned=0;
		}
	}
	return 0;
}

int set_table_core(int core, pid_t tid){
	int inserted=-1;
	for(int i=0;i<MAX_THREADS_PER_CORE;i++){
		if(core_table[core][i] < 0){
			core_table[core][i] = tid;
			inserted = i;
			break;
		}
	}
	return inserted;
}

//returns -1 if not free core
int find_first_free_core(){
	int ret;
	for(int i=0;i<SYS_NUM_OF_CORES;i++){
		ret=0;
		for(int j=0;j<MAX_THREADS_PER_CORE;j++){
			if(core_table[i][j] < 0){
				ret++;
			}
		}
		if(ret==MAX_THREADS_PER_CORE){//No threads in this core
			return i;
		}
	}
	return -1;//no free cores
}

int find_next_free_core(int core){
	int ret;
	for(int i=core;i<SYS_NUM_OF_CORES;i++){
		ret=0;
		for(int j=0;j<MAX_THREADS_PER_CORE;j++){
			if(core_table[i][j] < 0){
				ret++;
			}
		}
		if(ret==MAX_THREADS_PER_CORE){//
			return i;
		}
	}
	return -1;//no free cores
}


//INNEFICIENT!!!!!!!
int get_random_free_core(){
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

int pin_tid_to_core_and_set_table(int core, tid_cell_t *tid_c){
	CPU_ZERO(&(tid_c->affinity));
	CPU_SET(core,&(tid_c->affinity));
	tid_c->pinned=1;
	tid_c->assigned_core = core;
	set_table_core(core, tid_c->tid);
	return 0;
}

//returns -1 if no free spots, can be MAX_THREADS_PER_CORE
int pin_tid_to_first_free_core(tid_cell_t *tid_c){
	for(int j=0;j<MAX_THREADS_PER_CORE;j++){
		for(int i=0;i<SYS_NUM_OF_CORES;i++){
			if(core_table[i][j] < 0){
				pin_tid_to_core_and_set_table(i,tid_c);
				return i;
			}
		}
	}
	return -1;//no free cores
}

//returns -1 if no free spots, can be MAX_THREADS_PER_CORE
int pin_tid_to_core_or_first_free_core(int core, tid_cell_t *tid_c){
	for(int i=0;i<MAX_THREADS_PER_CORE;i++){
		if(core_table[core][i] < 0){
			pin_tid_to_core_and_set_table(core,tid_c);
			return core;
		}else{
			return pin_tid_to_first_free_core(tid_c);//preferred core not free
		}
	}
	return -1;//Should never be reached
}

int is_core_free(int core){
	int ret=0;
	for(int i=0;i<MAX_THREADS_PER_CORE;i++){
		if(core_table[core][i] < 0){
			ret++;
		}
	}
	if(ret==MAX_THREADS_PER_CORE){//No threads in this core
		return 1;
	}
	return 0;//core is not free
}


pid_t get_tid_in_core(int core, int thread_position){
	//sanity check, disabled
	/*
	if(thread_position>(MAX_THREADS_PER_CORE-1)){
		thread_position = 0;
	}*/
	return core_table[core][thread_position];
}

