#include "tid_page_accesses_list.h"

/*** tid_page_accesses_cell_t ***/
tid_page_accesses_cell_t::tid_page_accesses_cell(pid_t tid, float latency, int cpu){
	this->tid = tid;
	this->cpu = cpu;	
	mean_latency = latency;
	mean_latency_reset = latency;
	accesses = 1;
	accesses_reset = 1;
}

int tid_page_accesses_cell_t::update(float latency, int cpu){
	this->cpu = cpu;
	mean_latency = ((mean_latency * accesses) + latency) / (accesses+1);
	mean_latency_reset = ((mean_latency_reset * accesses_reset) + latency) / (accesses_reset+1);
	accesses++;
	accesses_reset++;
	return 2;
}

int tid_page_accesses_cell_t::reset(){
	mean_latency_reset = 0;
	accesses_reset = 0;
	return 2;
}

void tid_page_accesses_cell_t::print(){
	printf("TID: %d, CPU: %d, M_LAT %g, M_LAT_R %g, T_ACC %u, T_ACC_R %u\n", tid, cpu, mean_latency, mean_latency_reset, accesses, accesses_reset);
}

/*** tid_page_accesses_list_t ***/
int tid_page_accesses_list_t::reset(){
	for(int i=0;i<list.size();i++)
		list[i].reset();

	return 2;
}

int tid_page_accesses_list_t::add_cell(pid_t tid, float latency, int cpu){
	tid_page_accesses_cell_t *cell = get_cell(tid);
	tid_page_accesses_cell_t aux(tid,latency,cpu);

	if(cell==NULL)
		list.push_back(aux);
	else
		cell->update(latency,cpu);

	return 0;
}

tid_page_accesses_cell_t* tid_page_accesses_list_t::get_cell(pid_t tid){
	tid_page_accesses_cell_t *p;

	for(int i=0;i<list.size();i++){
		p = &list[i];
		if(p->tid == tid)
			return p;
	}

	return NULL;
}


void tid_page_accesses_list_t::clear(){
	list.clear();
}

void tid_page_accesses_list_t::print(){
	printf("tid page accesses list\n");

	if(list.empty())
		printf("empty\n");

	for(int i=0;i<list.size();i++)
		list[i].print();
}

