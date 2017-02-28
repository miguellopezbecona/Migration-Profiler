#include "sample_data.h"

void my_pebs_sample_t::print(FILE *fp){
	printf( "%#016lx ", iip);
	printf( "%d %d ", pid, tid);
	printf( "%lu ", time);
	printf( "%#016lx ", sample_addr);
	printf( "%u ", cpu);
	printf( "%lu ", weight);
	printf( "%lu %lu ", time_enabled, time_running);
	printf( "%#016lx", dsrc);
	for(int i=nr-1;i>=0;i--){
		printf( " %lu",values[i]);
	}
	printf("\n");
}

void my_pebs_sample_t::print_for_3DyRM(FILE *fp){
	// We use this format TYPE IIP PID TID TIME SAMPLE_ADDR CPU WEIGHT TIME_E TIME_R DSRC INST SSE_D SSE_S REQ_DR MEM_OPS
	if(nr==1){//it is a memory sample
		printf( "%c ",'M');
		printf( "%#016lx ", iip);
		printf( "%d %d ", pid, tid);
		printf( "%lu ", time);
		printf( "%#016lx ", sample_addr);
		printf( "%u ", cpu);
		printf( "%lu ", weight);
		printf( "%lu %lu ", time_enabled, time_running);
		printf( "%#016lx", dsrc);
		printf( " %lu",(long unsigned int)0);
		printf( " %lu",(long unsigned int)0);
		printf( " %lu",(long unsigned int)0);
		printf( " %lu",(long unsigned int)0);
		printf( " %lu",values[0]);
	}else{//it is an instruction sample
		printf( "%c ",'I');
		printf( "%#016lx ", iip);
		printf( "%d %d ", pid, tid);
		printf( "%lu ", time);
		printf( "%#016lx ", (long unsigned int)0); //Has no ADDR
		printf( "%u ", cpu);
		printf( "%lu ", (long unsigned int)0); // Has no WEIGHT
		printf( "%lu %lu ", time_enabled, time_running);
		printf( "%#016lx", (long unsigned int)0); //Has no DSRC
		for(int i=nr-1;i>=0;i--){//Should have INST SSE_D SSE_S REQ_DR
			printf( " %lu",values[i]);
		}
		printf( " %lu",(long unsigned int)0); //Has no MEM_OPS
	}
	printf("\n");
}

