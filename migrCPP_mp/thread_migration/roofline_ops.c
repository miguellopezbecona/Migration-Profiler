#include "roofline_ops.h"

#include <algorithm> // find

unsigned int current_step;
temp_tid_list_t main_temp_tid_list; //used to store inst data by tid, to make it easier to process.


int build_temp_tid_list(inst_data_list_t inst_list, memory_data_list_t *memory_list){
	int ret;
	if(inst_list.list.empty()){
		return -1;
	}

	inst_data_cell_t i_cell;
	for(int i=0;i<inst_list.list.size();i++){
		i_cell = inst_list.list[i];
		ret = main_temp_tid_list.add_cell(i_cell.cpu,i_cell.pid,i_cell.tid,i_cell.inst,i_cell.req_dr,i_cell.time);
	}

	if(memory_list==NULL){
		return 2;
	}

	memory_data_cell_t *m_cell;
	temp_tid_cell_t *temp_cell;
	for(int i=0;i<memory_list->list.size();i++){
		m_cell = &memory_list->list[i];
		temp_cell = main_temp_tid_list.get_cell(m_cell->tid);
		if(temp_cell!=NULL){//only if there is other data of this tid
			ret= temp_cell->update_latency(m_cell->cpu, m_cell->latency);
		} else {//ok, lets set data of only latency, this will count as a instruction sample, be careful. May not work
			ret = main_temp_tid_list.add_cell(m_cell->cpu,m_cell->pid,m_cell->tid,0,0,0);
			temp_cell = main_temp_tid_list.get_cell(m_cell->tid);//should exist now
			ret = temp_cell->update_latency(m_cell->cpu, m_cell->latency);
		}
	}

	return ret;	
}



int build_and_add_roofline_data_to_performance(temp_tid_cell_t temp_c, tid_cell_t* tid_c){
	double insts;
	double instb;
	float latency;
	for(int i=0;i<SYS_NUM_OF_CORES;i++){
		if(temp_c.elapsed[i]>0){//this means we have got data, else we do not
			insts = temp_c.inst[i] / (temp_c.elapsed[i]/1000.0); //lets do inst per ms

			if(insts!=insts) insts=0; //is NaN
			instb = 0;
			if(temp_c.req_dr[i] > 0){
				instb = temp_c.inst[i] / (temp_c.req_dr[i]*SYS_CACHE_LINE_SIZE*1.0);
			}
			if(temp_c.latency_samples[i]>0){
				latency = temp_c.latency[i] / temp_c.latency_samples[i];
			}else{
				latency=SYS_IMPOSSIBLE_LATENCY;
			}

			tid_c->update_performance(i,  current_step, temp_c.inst[i], (float)insts, (float)instb, latency);
		}
	}
	tid_c->last_core = temp_c.last_core;
	return 0;
}

int build_rooflines(pid_list_t* pid_m){
	int ret;
	
	temp_tid_cell_t cell;
	tid_l* tids = NULL;
	tid_cell_t* tid_c = NULL;

	if(main_temp_tid_list.list.empty())
		return -1; //only if we've got data

	for(int i=0;i<main_temp_tid_list.list.size();i++){
		cell = main_temp_tid_list.list[i];

		// Associates TID with PID if it was not done before
		tids = pid_m->get_tid_list(cell.pid);
		if(find(tids->begin(), tids->end(), cell.tid) == tids->end()) // If tids does not contain tid
			tids->push_back(cell.tid);

		// Inserts tid_cell in TID map if it does not exist
		if(!pid_m->has_tid_key(cell.tid))
			pid_m->tid_m.add_cell(cell.tid, cell.last_core);

		// Gets cell reference
		tid_c = pid_m->get_tid_cell(cell.tid);

		//builds roofline
		//printf("Updating to pid: %d, tid: %d\n",cell.pid, cell.tid);
		ret = build_and_add_roofline_data_to_performance(cell, tid_c);
	}

	return ret;
}

//the inst_data_list has to have the increments created
int rooflines(unsigned int step, memory_data_list_t* memory_list, inst_data_list_t inst_list, pid_list_t* pid_m){
	int ret;
	current_step=step;

	//do inst
	ret = build_temp_tid_list(inst_list, memory_list);

	//printf("temp_tid_list_created\n");
	//main_temp_tid_list.print();

	build_rooflines(pid_m);
	//printf("rooflines created\n");

	//clean up
	main_temp_tid_list.clear();
	return ret;
}
