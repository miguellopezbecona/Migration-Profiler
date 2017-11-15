#include "inst_list.h"

/*** inst_data_cell_t functions ***/
// Constructor
inst_data_cell_t::inst_data_cell(uint32_t cpu, int pid, int tid, uint64_t inst,uint64_t req_dr, uint64_t time){
	this->cpu=cpu;
	this->pid=pid;
	this->tid=tid;
	this->inst=inst;
	this->req_dr=req_dr;
	this->time=time;
}

void inst_data_cell_t::print(){
	printf("CPU: %u, PID: %d, TID: %d INST: %lu, REQ_DR: %lu TIME: %lu\n", cpu, pid, tid, inst, req_dr, time);
}

/*** inst_data_list_t functions ***/
void inst_data_list_t::add_cell(uint32_t cpu, int pid, int tid, uint64_t inst, uint64_t req_dr, uint64_t time){
	inst_data_cell_t cell(cpu, pid, tid, inst, req_dr, time);
	list.push_back(cell);
}

void inst_data_list_t::clear(){
	list.clear();
}

void inst_data_list_t::print(){
	for(size_t i=0;i<list.size();i++)
		list[i].print();
}

/*
 * Builds the increment of the instruction counts, cpu by cpu
 */
int inst_data_list_t::create_increments(){
	if(list.empty()) return -1;

	uint64_t last_inst[system_struct_t::NUM_OF_CPUS];
	uint64_t last_req_dr[system_struct_t::NUM_OF_CPUS];
	uint64_t last_time[system_struct_t::NUM_OF_CPUS];
	uint64_t temp_inst;
	uint64_t temp_req_dr;
	uint64_t temp_time;

	// Zero initialization
	memset(last_inst, 0, sizeof(uint64_t)*system_struct_t::NUM_OF_CPUS);
	memset(last_req_dr, 0, sizeof(uint64_t)*system_struct_t::NUM_OF_CPUS);
	memset(last_time, 0, sizeof(uint64_t)*system_struct_t::NUM_OF_CPUS);
			
	//now let us suppose data are ordered by time FOR EACH CPU
	inst_data_cell_t* cell;
	for(size_t i=0;i<list.size();i++){
		cell = &list.at(i);
		temp_inst = cell->inst;
		temp_req_dr = cell->req_dr;
		temp_time = cell->time;
		if(last_time[cell->cpu]==0){ // it is the first datum of the cpu, set to zero, should be taken care when building the roofline
			cell->inst=0;
			cell->req_dr=0;
			cell->time=0;
		} else {
			cell->inst -= last_inst[cell->cpu];
			cell->req_dr -= last_req_dr[cell->cpu];
			cell->time -= last_time[cell->cpu];	
		}
		last_inst[cell->cpu] = temp_inst;
		last_req_dr[cell->cpu] = temp_req_dr;
		last_time[cell->cpu] = temp_time;
	}
	return 0;
}
