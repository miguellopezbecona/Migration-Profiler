#include "tid_list.h"

/*** tid_cell_t functions ***/
// Constructor
tid_cell_t::tid_cell(pid_t tid, uint32_t core){
	this->tid = tid;
	last_core=core;
	assigned_core=-1;
	pinned = false;
	step_last_migrated=0;
	active = false;
	int ret = sched_getaffinity(tid, sizeof(cpu_set_t), &affinity);
	
	tid_performance_t p;
	performance = p;
}

void tid_cell_t::print(){
	printf("CORE(l,a): %d,%d,", last_core, assigned_core);
	if(!pinned){
		printf("free,");
	}else{
		printf("pinned,");
	}

	if(!active){
		printf("inactive\n");
	}else{
		printf("active\n");
	}

	performance.print();
}

double tid_cell_t::get_last_performance(){
	if(active)
		return performance.last_DyRM_performance;
	else
		return 0;
}

int tid_cell_t::update_performance(int core, unsigned int step, unsigned int instructions, float flops, float flopb, float latency){
	active = true;
	return performance.update(core, step, instructions, flops, flopb, latency);
}


/*** tid_list_t functions ***/
void tid_list_t::add_cell(pid_t tid, uint32_t core){
	tid_cell_t cell(tid, core);
	map[tid] = cell;
}

tid_cell_t* tid_list_t::get_cell(pid_t tid){
	return &map[tid];
}

int tid_list_t::reset_performance(){
	tid_cell_t* cell = NULL;
	int ret = -1;
	for(tr1::unordered_map<pid_t, tid_cell_t>::iterator it = map.begin(); it != map.end(); ++it) {
		cell = &it->second;
		cell->active = false;
		ret = cell->performance.reset();
	}

	return ret;
}

int tid_list_t::set_inactive(){
	tid_cell_t* cell = NULL;
	int ret = -1;
	for(tr1::unordered_map<pid_t, tid_cell_t>::iterator it = map.begin(); it != map.end(); ++it) {
		cell = &it->second;
		cell->active = false;
	}

	return ret;
}

void tid_list_t::clear(){
	map.clear();
}

void tid_list_t::print(){
	printf("tid map: \n");
	if(map.empty()){
		printf("empty\n");
		return;
	}

	tid_cell_t cell;
	for(tr1::unordered_map<pid_t, tid_cell_t>::iterator it = map.begin(); it != map.end(); ++it) {
		printf("TID: %u, ", it->first);
		cell = it->second;
		cell.print();
	}
}

tid_cell_t* tid_list_t::get_cell_assigned_in_core(int core){
	tid_cell_t* cell = NULL;
	for(tr1::unordered_map<pid_t, tid_cell_t>::iterator it = map.begin(); it != map.end(); ++it) {
		cell = &it->second;
		if(cell->assigned_core == core)
			return cell;
	}

	return NULL;
}

