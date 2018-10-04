#include "sample_data.h"

my_pebs_sample_t::my_pebs_sample() {
	#ifdef JUST_PROFILE_ENERGY
	energies = (double*)malloc(energy_data_t::NUM_RAPL_DOMAINS*sizeof(double));

	for(int i=0;i<energy_data_t::NUM_RAPL_DOMAINS;i++)
		energies[i] = -1.0;
	#endif
}

bool my_pebs_sample_t::is_mem_sample() const {
	return nr == 1; //sample_addr != 0
}

#ifdef JUST_PROFILE
int my_pebs_sample_t::max_nr = 0;
vector<char*> my_pebs_sample_t::inst_subevent_names;

void my_pebs_sample_t::print(FILE *fp) const {
	char type = 'I';
	bool mem_sample = is_mem_sample();
	if(mem_sample) // Memory sample format
		type = 'M';

	#ifdef SIMPL_PRINT
	fprintf(fp, "%c,%d,%d,%u,%lu,%#lx,%lu", type, pid, tid, cpu, time, sample_addr, weight);
	#else
	fprintf(fp, "%c,%#lx,%d,%d,%u,%lu,%#lx,%lu,%lu,%lu,%#lx", type, iip, pid, tid, cpu, time, sample_addr, weight,time_enabled, time_running,dsrc);
	#endif

	if(mem_sample){
		for(int i=0;i<max_nr;i++) // No inst subevents
			fprintf(fp, ",0");
		#ifndef SIMPL_PRINT
		fprintf(fp, ",%lu",values[0]); // Mem ops
		#endif
	} else {
		for(int i=nr-1;i>=0;i--) // Inst subevents
			fprintf(fp, ",%lu", values[i]);
		#ifndef SIMPL_PRINT
		fprintf(fp, ",%d", 0); // No MEM_OPS
		#endif
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

	// Copies energy values from the associated node to CPU sample
	memcpy(energies, vals[node], energy_data_t::NUM_RAPL_DOMAINS*sizeof(double));
}
#endif

void my_pebs_sample_t::add_subevent_name(char* name) {
	inst_subevent_names.push_back(strdup(name));
}

void my_pebs_sample_t::print_header(FILE *fp) {
	#ifdef SIMPL_PRINT
	fprintf(fp, "TYPE,PID,TID,CPU,TIME,SAMPLE_ADDR,WEIGHT");
	#else
	fprintf(fp, "TYPE,IIP,PID,TID,CPU,TIME,SAMPLE_ADDR,WEIGHT,TIME_E,TIME_R,DSRC");
	#endif

	for(char* const & e_name : inst_subevent_names)
		fprintf(fp, ",%s", e_name);

	#ifndef SIMPL_PRINT
	fprintf(fp, ",MEM_OPS");
	#endif

	#ifndef JUST_PROFILE_ENERGY
	fprintf(fp, "\n");
	#else
	for(char* const & d_name : energy_data_t::rapl_domain_names)
		fprintf(fp, ",%s", d_name);
	fprintf(fp, "\n");
	#endif	
}
#endif
