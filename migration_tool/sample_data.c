#include "sample_data.h"

bool my_pebs_sample_t::is_mem_sample() const {
	return nr == 1; //sample_addr != 0
}

void my_pebs_sample_t::print(FILE *fp) const {
	fprintf(fp, "%#016lx ", iip);
	fprintf(fp, "%d %d ", pid, tid);
	fprintf(fp, "%lu ", time);
	fprintf(fp, "%#016lx ", sample_addr);
	fprintf(fp, "%u ", cpu);
	fprintf(fp, "%lu ", weight);
	fprintf(fp, "%lu %lu ", time_enabled, time_running);
	fprintf(fp, "%#016lx", dsrc);
	for(int i=nr-1;i>=0;i--)
		fprintf(fp, " %lu",values[i]);
	fprintf(fp, "\n");
}

void my_pebs_sample_t::print_for_3DyRM(FILE *fp) const {
	if(is_mem_sample()){ // Memory sample format
		fprintf(fp, "%c,", 'M');
		fprintf(fp, "%#016lx,", iip);
		fprintf(fp, "%d,%d,", pid, tid);
		fprintf(fp, "%lu,", time);
		fprintf(fp, "%#016lx,", sample_addr);
		fprintf(fp, "%u,", cpu);
		fprintf(fp, "%lu,", weight);
		fprintf(fp, "%lu,%lu,", time_enabled, time_running);
		fprintf(fp, "%#016lx,", dsrc);
		fprintf(fp, "%d,", 0); // No INST data
		fprintf(fp, "%d,", 0); // No REQ_DR

		// More zeros can be added depending on instruction additional fields (SSE_D, SSE_S...)
		/*
		fprintf(fp, "%d,", 0);
		fprintf(fp, "%d,", 0);
		*/
		fprintf(fp, "%lu",values[0]);
	} else { // Instruction sample format
		fprintf(fp, "%c,", 'I');
		fprintf(fp, "%#016lx,", iip);
		fprintf(fp, "%d,%d,", pid, tid);
		fprintf(fp, "%lu,", time);
		fprintf(fp, "%x,", 0); // has no ADDR
		fprintf(fp, "%u,", cpu);
		fprintf(fp, "%d,", 0); // Has no WEIGHT
		fprintf(fp, "%lu,%lu,", time_enabled, time_running);
		fprintf(fp, "%x,", 0); // has no DSRC
		for(int i=nr-1;i>=0;i--) // Right now we have just INST and REQ_DR. SSE_D, SSE_S could be added
			fprintf(fp, "%lu,", values[i]);
		fprintf(fp, "%d", 0); // Has no MEM_OPS
	}

	#ifdef JUST_PROFILE_ENERGY
	for(int i=0;i<energy_data_t::NUM_RAPL_DOMAINS;i++)
		fprintf(fp, ",%.3f", energies[i]);
	#endif

	fprintf(fp, "\n");
}

#ifdef JUST_PROFILE_ENERGY
void my_pebs_sample_t::add_energy_data() {
	int node = system_struct_t::cpu_node_map[cpu];
	double** vals = ed.get_curr_vals(); // Energy values

	energies = (double*)malloc(energy_data_t::NUM_RAPL_DOMAINS*sizeof(double));

	// Copies energy values from the associated node to CPU sample
	memcpy(energies, vals[node], energy_data_t::NUM_RAPL_DOMAINS*sizeof(double));
}
#endif

void my_pebs_sample_t::print_header(FILE *fp) {
	#ifndef JUST_PROFILE_ENERGY
	fprintf(fp, "TYPE,IIP,PID,TID,TIME,SAMPLE_ADDR,CPU,WEIGHT,TIME_E,TIME_R,DSRC,INST,REQ_DR,MEM_OPS\n");
	#else
	fprintf(fp, "TYPE,IIP,PID,TID,TIME,SAMPLE_ADDR,CPU,WEIGHT,TIME_E,TIME_R,DSRC,INST,REQ_DR,MEM_OPS");
	for(char* const & d_name : energy_data_t::rapl_domain_names)
		fprintf(fp, ",%s", d_name);
	fprintf(fp, "\n");
	#endif	
}
