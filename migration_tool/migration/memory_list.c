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

void memory_data_cell_t::print() const {
	printf("CPU: %u, PID: %d, TID: %d ADDR: %#016lx, LATENCY: %lu, DSRC: %#016lx, TIME: %lu\n", cpu, pid, tid, addr, latency, dsrc, time);
}

bool memory_data_cell_t::is_cache_miss() const {
	union perf_mem_data_src *mdsrc = (perf_mem_data_src*) &dsrc;

	// PERF_MEM_LVL_MISS or PERF_MEM_LVL_UNC?
	//printf("%d %u, ", mdsrc->mem_lvl & PERF_MEM_LVL_MISS, mdsrc->mem_lvl & PERF_MEM_LVL_UNC);
	return mdsrc->mem_lvl & PERF_MEM_LVL_MISS;
}

void memory_data_cell_t::print_dsrc() const {
	union perf_mem_data_src *mdsrc = (perf_mem_data_src*) &dsrc;

	printf("mem_op: %lu ->\n", mdsrc->mem_op);
	if (mdsrc->mem_op&PERF_MEM_OP_NA)			printf("\tnot available\n");
	if (mdsrc->mem_op&PERF_MEM_OP_LOAD) 			printf("\tload instruction\n");
	if (mdsrc->mem_op&PERF_MEM_OP_STORE) 			printf("\tstore instruction\n");
	if (mdsrc->mem_op&PERF_MEM_OP_PFETCH)			printf("\tprefetch\n");
	if (mdsrc->mem_op&PERF_MEM_OP_EXEC) 			printf("\tcode (execution)\n");

	printf("mem_lvl: %lu ->\n", mdsrc->mem_lvl);
	if (mdsrc->mem_lvl&PERF_MEM_LVL_NA)			printf("\tnot available\n");
	if (mdsrc->mem_lvl&PERF_MEM_LVL_HIT)			printf("\thit level\n");
	if (mdsrc->mem_lvl&PERF_MEM_LVL_MISS)			printf("\tmiss level\n");
	if (mdsrc->mem_lvl&PERF_MEM_LVL_L1)			printf("\tL1\n");
	if (mdsrc->mem_lvl&PERF_MEM_LVL_LFB)			printf("\tLine Fill Buffer\n");
	if (mdsrc->mem_lvl&PERF_MEM_LVL_L2)			printf("\tL2\n");
	if (mdsrc->mem_lvl&PERF_MEM_LVL_L3)			printf("\tL3\n");
	if (mdsrc->mem_lvl&PERF_MEM_LVL_LOC_RAM)			printf("\tLocal DRAM\n");
	if (mdsrc->mem_lvl&PERF_MEM_LVL_REM_RAM1)		printf("\tRemote DRAM (1 hop)\n");
	if (mdsrc->mem_lvl&PERF_MEM_LVL_REM_RAM2)		printf("\tRemote DRAM (2 hops)\n");
	if (mdsrc->mem_lvl&PERF_MEM_LVL_REM_CCE1)		printf("\tRemote Cache (1 hop)\n");
	if (mdsrc->mem_lvl&PERF_MEM_LVL_REM_CCE2)		printf("\tRemote Cache (2 hops)\n");
	if (mdsrc->mem_lvl&PERF_MEM_LVL_IO)			printf("\tI/O memory\n");
	if (mdsrc->mem_lvl&PERF_MEM_LVL_UNC)			printf("\tUncached memory\n");

	printf("mem_snoop: %lu ->\n", mdsrc->mem_snoop);
	if (mdsrc->mem_snoop&PERF_MEM_SNOOP_NA)			printf("\tnot available\n");
	if (mdsrc->mem_snoop&PERF_MEM_SNOOP_NONE)		printf("\tno snoop\n");
	if (mdsrc->mem_snoop&PERF_MEM_SNOOP_HIT)			printf("\tsnoop hit\n");
	if (mdsrc->mem_snoop&PERF_MEM_SNOOP_MISS)		printf("\tsnoop miss\n");
	if (mdsrc->mem_snoop&PERF_MEM_SNOOP_HITM)		printf("\tsnoop hit modified\n");
	
	printf("mem_lock: %lu ->\n", mdsrc->mem_lock);
	if (mdsrc->mem_lock&PERF_MEM_LOCK_NA)			printf("\tnot available\n");
	if (mdsrc->mem_lock&PERF_MEM_LOCK_LOCKED)		printf("\tlocked transaction\n");

	printf("mem_dtlb: %lu ->\n", mdsrc->mem_dtlb);
	if (mdsrc->mem_dtlb&PERF_MEM_TLB_NA)			printf("\tnot available\n");
	if (mdsrc->mem_dtlb&PERF_MEM_TLB_HIT)			printf("\thit level\n");
	if (mdsrc->mem_dtlb&PERF_MEM_TLB_MISS)			printf("\tmiss level\n");
	if (mdsrc->mem_dtlb&PERF_MEM_TLB_L1)			printf("\tL1\n");
	if (mdsrc->mem_dtlb&PERF_MEM_TLB_L2)			printf("\tL2\n");
	if (mdsrc->mem_dtlb&PERF_MEM_TLB_WK)			printf("\tHardware Walker\n");
	if (mdsrc->mem_dtlb&PERF_MEM_TLB_OS)			printf("\tOS fault handler\n");
}

/*** memory_data_list_t functions ***/
void memory_data_list_t::add_cell(uint32_t cpu, int pid, int tid, uint64_t addr, uint64_t latency, uint64_t dsrc, uint64_t time){
	memory_data_cell_t cell(cpu, pid, tid, addr, latency, dsrc, time);
	list.push_back(cell);
}

bool memory_data_list_t::is_empty() {
	return list.empty();
}

void memory_data_list_t::clear(){
	list.clear();
}

void memory_data_list_t::print() {
	for(size_t i=0;i<list.size();i++)
		list[i].print();
}
