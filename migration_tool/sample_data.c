#include "sample_data.h"

bool my_pebs_sample_t::is_mem_sample() const {
	return sample_addr != 0; // Before implemented as nr == 1
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

		// More zeros can be added depending on instruction additional fields
		/*
		fprintf(fp, "%d,", 0);
		fprintf(fp, "%d,", 0);
		fprintf(fp, "%d,", 0);
		*/
		fprintf(fp, "%lu",values[0]);
	} else { // Instruction sample format
		fprintf(fp, "%c,", 'I');
		fprintf(fp, "%#016lx,", iip);
		fprintf(fp, "%d,%d,", pid, tid);
		fprintf(fp, "%lu,", time);
		fprintf(fp, "%#016lx,", (long unsigned int) 0); // has no ADDR
		fprintf(fp, "%u,", cpu);
		fprintf(fp, "%d,", 0); // Has no WEIGHT
		fprintf(fp, "%lu,%lu,", time_enabled, time_running);
		fprintf(fp, "%#016lx,", (long unsigned int) 0); // has no DSRC
		for(int i=nr-1;i>=0;i--) // Right now we have just INST. SSE_D, SSE_S and REQ_DR could be added
			fprintf(fp, "%lu,", values[i]);
		fprintf(fp, "%d", 0); // Has no MEM_OPS
	}
	fprintf(fp, "\n");
}

void my_pebs_sample_t::print_header(FILE *fp) {
	fprintf(fp, "TYPE,IIP,PID,TID,TIME,SAMPLE_ADDR,CPU,WEIGHT,TIME_E,TIME_R,DSRC,INST,MEM_OPS\n");
}
