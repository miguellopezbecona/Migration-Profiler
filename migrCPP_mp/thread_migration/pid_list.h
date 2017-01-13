#ifndef __PID_LIST_H__
#define __PID_LIST_H__

#include <stdio.h>
#include <inttypes.h>

#include "tid_list.h"

#include <vector>
#include <tr1/unordered_map>
using namespace std;

// For each PID, you get a list of TIDs (numeric values). You can get the corresponding "tid_cell_t"s mapping in tid_m, so the access is nearly constant
typedef vector<pid_t> tid_l;
typedef struct pid_list{
	tr1::unordered_map<pid_t, tid_l> map; // PID map
	tid_list_t tid_m; // Object that encapsulates TID map

	bool is_empty();
	tid_l* get_tid_list(pid_t pid);
	tid_cell_t* get_tid_cell(pid_t tid);
	bool has_tid_key(pid_t tid);
	void clear();

	void print();
	void print_redux();

	double calculate_total_last_performance_from_pid(pid_t pid);

	//does last_DyRM_performance/mean_last_DyRM_performance for all ACTIVE and returns worst. MAY BE NULL IF ALL INACTIVE!
	tid_cell_t* normalise_all_last_DyRM_to_mean_by_pid_and_return_worst(pid_t pid);
} pid_list_t;

#endif
