#include "memory_list.h"
#include "../perfmon/perf_util.h" // For is_cache_miss

/*** memory_data_cell_t functions ***/
// Constructor
memory_data_cell_t::memory_data_cell(uint32_t cpu, int pid, int tid, uint64_t addr, uint64_t latency, uint64_t dsrc, uint64_t time){
	this->cpu=cpu;
	this->pid=pid;
	this->tid=tid;
	this->addr=addr;
	this->latency=latency;
	this->dsrc=dsrc;
	this->time=time;
}

void memory_data_cell_t::print(){
	printf("CPU: %u, PID: %d, TID: %d ADDR: %#016lx, LATENCY: %lu, DSRC: %#016lx, TIME: %lu\n", cpu, pid, tid, addr, latency, dsrc, time);
}

bool memory_data_cell_t::is_cache_miss(){
	union perf_mem_data_src *mdsrc = (perf_mem_data_src*) &dsrc;

	// PERF_MEM_LVL_MISS or PERF_MEM_LVL_UNC?
	//printf("%d %u, ", mdsrc->mem_lvl & PERF_MEM_LVL_MISS, mdsrc->mem_lvl & PERF_MEM_LVL_UNC);
	return mdsrc->mem_lvl & PERF_MEM_LVL_MISS;
}

/*** memory_data_list_t functions ***/
void memory_data_list_t::add_cell(uint32_t cpu, int pid, int tid, uint64_t addr, uint64_t latency, uint64_t dsrc, uint64_t time){
	memory_data_cell_t cell(cpu, pid, tid, addr, latency, dsrc, time);
	list.push_back(cell);
}

bool memory_data_list_t::is_empty(){
	return list.empty();
}

void memory_data_list_t::clear(){
	list.clear();
}

void memory_data_list_t::print(){
	for(size_t i=0;i<list.size();i++)
		list[i].print();
}
