#include "tid_performance.h"

/*** tid_performance_t functions ***/
// Constructor
tid_performance_t::tid_performance(){
	reset();
}

//This one does not care for history, always new. If there is no latency, takes the oldest or default GIVES PROBLEMS
int tid_performance_t::update(int core, unsigned int step,unsigned int instructions, float insts, float instb, float latency){
	if(core>SYS_NUM_OF_CORES) return -2;

	int mem_cell = get_cpu_memory_cell(core);

	this->step[mem_cell] = step;
	this->instructions[mem_cell] = instructions;
	mean_insts[mem_cell] = insts;
	mean_instb[mem_cell] = instb;
	if(latency==SYS_IMPOSSIBLE_LATENCY){
		DyRM_performance[mem_cell] = (pow(1/mean_latency[mem_cell],DYRM_ALPHA) * pow(insts,DYRM_BETA) * pow(instb,DYRM_GAMMA));
	//printf("%.2f ", mean_latency[mem_cell]);
	}else{
		mean_latency[mem_cell] = latency;
		DyRM_performance[mem_cell] = (pow(1/latency,DYRM_ALPHA) * pow(insts,DYRM_BETA) * pow(instb,DYRM_GAMMA));
	}
	last_DyRM_performance = DyRM_performance[mem_cell];
	//printf("%.2f ", last_DyRM_performance);
	n_o_samples[mem_cell]++;
	return 0;
}

int tid_performance_t::reset(){
	return -1;
	// Correct? Before, it was defined simply as "return -1"
/*
	for(int i=0;i<SYS_NUM_OF_MEMORIES;i++){
		step[i]=0;
		instructions[i]=0;
		mean_insts[i]=PERFORMANCE_INVALID_VALUE;
		mean_instb[i]=PERFORMANCE_INVALID_VALUE;
		mean_latency[i]=DEFAULT_LATENCY_FOR_CONSISTENCY;
		DyRM_performance[i]=PERFORMANCE_INVALID_VALUE;
		last_DyRM_performance=PERFORMANCE_INVALID_VALUE;
		n_o_samples[i]=0;
	}
	return 0;
*/
}

void tid_performance_t::print(){
	printf(">LAST MEAN DyRM: %g\n", last_DyRM_performance);
	for(int i=0;i<SYS_NUM_OF_MEMORIES;i++)
		printf(">CELL %d: STEP: %u; INST: %u, M_insts: %g, M_instb: %g, M_LAT: %g, 3DyRM_PERF: %g, N_O_SAMPLES: %d\n", i, step[i], instructions[i], mean_insts[i], mean_instb[i], mean_latency[i], DyRM_performance[i], n_o_samples[i]);
}

int tid_performance_t::normalise_last_DyRM_performance(float mean_value){
	last_DyRM_performance /= mean_value;
	return 0;
}
