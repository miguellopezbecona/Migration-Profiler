#include "pid_list.h"

/*** pid_list_t functions ***/
bool pid_list_t::is_empty(){
	return map.empty();
}

// Getting an element (list of numeric TIDs) in PID map
tid_l* pid_list_t::get_tid_list(pid_t pid){
	return &map[pid];
}

// Getting an element (tid_cell) in TID map
tid_cell_t* pid_list_t::get_tid_cell(pid_t tid){
	return &tid_m.map[tid];
}

// tid_map.contains(tid) can be implemented by counting the key
bool pid_list_t::has_tid_key(pid_t tid){
	return tid_m.map.count(tid) > 0;
}

double pid_list_t::calculate_total_last_performance_from_pid(pid_t pid){
	double total_perf = 0.0;

	tid_l tids = map[pid]; // Gets TIDs associated to PID
	for(int i=0;i<tids.size();i++){
		pid_t tid = tids[i];
		total_perf += tid_m.map[tid].get_last_performance();
	}

	return total_perf;
}


tid_cell_t* pid_list_t::normalise_all_last_DyRM_to_mean_by_pid_and_return_worst(pid_t pid){
	if(map.empty())
		return NULL;

	double mean_perf = 0.0;
	double worst_perf = 99999.0;
	int active_threads = 0;

	tid_cell_t *worst_cell = NULL;
	tid_cell_t *cell;

	tid_l tids = map[pid]; // Gets TIDs associated to PID
	for(int i=0;i<tids.size();i++){
		pid_t tid = tids[i];
		tid_cell_t* cell = &tid_m.map[tid]; // Gets tid_cell_t associated to TID

		//only if active!!
		if(cell->active){
			mean_perf = mean_perf + cell->performance.last_DyRM_performance;
			active_threads++;
		}
	}

	//printf("Normalising %d with %d active threads, total perf %g\n", pid, active_threads, mean_perf);
	//only if there are active threads
	if(active_threads > 0){
		mean_perf = mean_perf / active_threads;

		// Normalise
		for(int i=0;i<tids.size();i++){
			pid_t tid = tids[i];
			tid_cell_t* cell = &tid_m.map[tid]; // Gets tid_cell_t associated to TID

			//only if active!!
			if(cell->active){
				cell->performance.normalise_last_DyRM_performance(mean_perf);

				//find worst
				if(cell->performance.last_DyRM_performance < worst_perf){
					worst_perf = cell->performance.last_DyRM_performance;
					worst_cell = cell;
				}
			}
		}
	}

	return worst_cell;
}

void pid_list_t::clear(){
	map.clear();
}

void pid_list_t::print(){
	printf("pid map: \n");
	if(map.empty()){
		printf("empty\n");
		return;
	}

	// For each entry, prints PID and associated "tid_cell_t"s
	pid_t pid;
	tid_l list;
	for(tr1::unordered_map<pid_t, tid_l>::iterator it = map.begin(); it != map.end(); ++it) {
		pid = it->first;
		list = it->second;

		printf("PID: %d\n", pid);
		for(int i=0;i<list.size();i++)
			tid_m.map[ list[i] ];
	}
}

void pid_list_t::print_redux(){
	printf("pid map redux: \n");
	if(map.empty()){
		printf("empty\n");
		return;
	}

	// For each entry, prints PID and associated TIDs
	pid_t pid;
	tid_l list;
	for(tr1::unordered_map<pid_t, tid_l>::iterator it = map.begin(); it != map.end(); ++it) {
		pid = it->first;
		list = it->second;

		printf("PID: %d\n", pid);
		for(int i=0;i<list.size();i++){
			printf("  TID: %d\n", list[i]);
		}
	}
}
