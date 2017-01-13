#include "temp_tid_list.h"

/*** temp_tid_cell_t functions ***/
// Constructor
temp_tid_cell_t::temp_tid_cell(uint32_t cpu, pid_t pid, pid_t tid, uint64_t inst, uint64_t req_dr, uint64_t elapsed){
	this->pid=pid;
	this->tid=tid;
	this->last_core=cpu;
	for(int i=0;i<SYS_NUM_OF_CORES;i++){
		this->inst[i]=0;
		this->req_dr[i]=0;
		this->elapsed[i]=0;
		this->latency[i]=0;
		this->latency_samples[i]=0;
	}
	this->inst[cpu]=inst;
	this->req_dr[cpu]=req_dr;
	this->elapsed[cpu]=elapsed;
	this->n_o_inst_samples[cpu]++;
	//this->samples=1;
}

void temp_tid_cell_t::print(){
	printf("CPU: %u, PID %d, TID: %d\n", last_core, pid, tid);
}

int temp_tid_cell_t::update_cell(uint32_t cpu, uint64_t inst, uint64_t req_dr, uint64_t elapsed){
	//this->samples++;
	this->last_core = cpu;
	this->inst[cpu] += inst;
	this->req_dr[cpu] += req_dr;
	this->elapsed[cpu] += elapsed;
	this->n_o_inst_samples[cpu]++;
	return 0;
}

int temp_tid_cell_t::update_latency(uint32_t cpu, uint64_t latency){
	this->latency[cpu] += latency;
	this->latency_samples[cpu]++;
}

/*** temp_tid_list_t functions ***/
int temp_tid_list_t::add_cell(uint32_t cpu, pid_t pid, pid_t tid, uint64_t inst, uint64_t req_dr, uint64_t elapsed){
	temp_tid_cell_t aux(cpu, pid, tid, inst, req_dr, elapsed);
	temp_tid_cell_t *cell = get_cell(tid);

	// If the cell doesn't exist, inserts it. If it does, updates it
	if(cell==NULL)
		list.push_back(aux);
	else
		cell->update_cell(cpu, inst, req_dr, elapsed);

	return 0;
}

temp_tid_cell_t* temp_tid_list_t::get_cell(pid_t tid){
	if(list.empty())
		return NULL;

	temp_tid_cell_t *cell;
	for(int i=0;i<list.size();i++){
		cell = &list[i];
		if(cell->tid == tid){
			return cell;
		}
	}

	return NULL;
}

void temp_tid_list_t::clear(){
	list.clear();
}

void temp_tid_list_t::print(){
	printf("temp tid list\n");
	if(list.empty()){
		printf("empty\n");
		return;
	}

	for(int i=0;i<list.size();i++)
		list[i].print();
}
