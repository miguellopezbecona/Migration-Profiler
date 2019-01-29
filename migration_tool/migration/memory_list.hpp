#pragma once
#ifndef __MEMORY_LIST__
#define __MEMORY_LIST__

#include <iostream>
#include <cinttypes>

#include <unistd.h>

#include <vector>

class memory_data_cell_t {
public:
	uint32_t cpu;
	int pid;
	int tid;
	uint64_t addr;
	uint64_t latency;
	uint64_t dsrc;
	uint64_t time;

	memory_data_cell_t () {};

	memory_data_cell_t (const uint32_t cpu, const int pid, const int tid, const uint64_t addr, const uint64_t latency, const uint64_t dsrc, const uint64_t time) :
		cpu(cpu),
		pid(pid),
		tid(tid),
		addr(addr),
		latency(latency),
		dsrc(dsrc),
		time(time)
	{};

	inline bool is_cache_miss () const {
		union perf_mem_data_src * mdsrc = (perf_mem_data_src *) &dsrc;

		// PERF_MEM_LVL_MISS or PERF_MEM_LVL_UNC?
		//printf("%d %u, ", mdsrc->mem_lvl & PERF_MEM_LVL_MISS, mdsrc->mem_lvl & PERF_MEM_LVL_UNC);
		return mdsrc->mem_lvl & PERF_MEM_LVL_MISS;
	}

	void print_dsrc () const {
		union perf_mem_data_src * mdsrc = (perf_mem_data_src*) &dsrc;

		std::cout << "mem_op: " << mdsrc->mem_op << " ->\n";
		if (mdsrc->mem_op&PERF_MEM_OP_NA)			std::cout << "\tnot available\n";
		if (mdsrc->mem_op&PERF_MEM_OP_LOAD) 		std::cout << "\tload instruction\n";
		if (mdsrc->mem_op&PERF_MEM_OP_STORE) 		std::cout << "\tstore instruction\n";
		if (mdsrc->mem_op&PERF_MEM_OP_PFETCH)		std::cout << "\tprefetch\n";
		if (mdsrc->mem_op&PERF_MEM_OP_EXEC) 		std::cout << "\tcode (execution)\n";

		std::cout << "mem_lvl: " << mdsrc->mem_lvl << " ->\n";
		if (mdsrc->mem_lvl&PERF_MEM_LVL_NA)			std::cout << "\tnot available\n";
		if (mdsrc->mem_lvl&PERF_MEM_LVL_HIT)		std::cout << "\thit level\n";
		if (mdsrc->mem_lvl&PERF_MEM_LVL_MISS)		std::cout << "\tmiss level\n";
		if (mdsrc->mem_lvl&PERF_MEM_LVL_L1)			std::cout << "\tL1\n";
		if (mdsrc->mem_lvl&PERF_MEM_LVL_LFB)		std::cout << "\tLine Fill Buffer\n";
		if (mdsrc->mem_lvl&PERF_MEM_LVL_L2)			std::cout << "\tL2\n";
		if (mdsrc->mem_lvl&PERF_MEM_LVL_L3)			std::cout << "\tL3\n";
		if (mdsrc->mem_lvl&PERF_MEM_LVL_LOC_RAM)	std::cout << "\tLocal DRAM\n";
		if (mdsrc->mem_lvl&PERF_MEM_LVL_REM_RAM1)	std::cout << "\tRemote DRAM (1 hop)\n";
		if (mdsrc->mem_lvl&PERF_MEM_LVL_REM_RAM2)	std::cout << "\tRemote DRAM (2 hops)\n";
		if (mdsrc->mem_lvl&PERF_MEM_LVL_REM_CCE1)	std::cout << "\tRemote Cache (1 hop)\n";
		if (mdsrc->mem_lvl&PERF_MEM_LVL_REM_CCE2)	std::cout << "\tRemote Cache (2 hops)\n";
		if (mdsrc->mem_lvl&PERF_MEM_LVL_IO)			std::cout << "\tI/O memory\n";
		if (mdsrc->mem_lvl&PERF_MEM_LVL_UNC)		std::cout << "\tUncached memory\n";

		std::cout << "mem_snoop: " << mdsrc->mem_snoop << " ->\n";
		if (mdsrc->mem_snoop&PERF_MEM_SNOOP_NA)		std::cout << "\tnot available\n";
		if (mdsrc->mem_snoop&PERF_MEM_SNOOP_NONE)	std::cout << "\tno snoop\n";
		if (mdsrc->mem_snoop&PERF_MEM_SNOOP_HIT)	std::cout << "\tsnoop hit\n";
		if (mdsrc->mem_snoop&PERF_MEM_SNOOP_MISS)	std::cout << "\tsnoop miss\n";
		if (mdsrc->mem_snoop&PERF_MEM_SNOOP_HITM)	std::cout << "\tsnoop hit modified\n";

		std::cout << "mem_lock: " << mdsrc->mem_lock << " ->\n";
		if (mdsrc->mem_lock&PERF_MEM_LOCK_NA)		std::cout << "\tnot available\n";
		if (mdsrc->mem_lock&PERF_MEM_LOCK_LOCKED)	std::cout << "\tlocked transaction\n";

		std::cout << "mem_dtlb: " << mdsrc->mem_dtlb << " ->\n";
		if (mdsrc->mem_dtlb&PERF_MEM_TLB_NA)		std::cout << "\tnot available\n";
		if (mdsrc->mem_dtlb&PERF_MEM_TLB_HIT)		std::cout << "\thit level\n";
		if (mdsrc->mem_dtlb&PERF_MEM_TLB_MISS)		std::cout << "\tmiss level\n";
		if (mdsrc->mem_dtlb&PERF_MEM_TLB_L1)		std::cout << "\tL1\n";
		if (mdsrc->mem_dtlb&PERF_MEM_TLB_L2)		std::cout << "\tL2\n";
		if (mdsrc->mem_dtlb&PERF_MEM_TLB_WK)		std::cout << "\tHardware Walker\n";
		if (mdsrc->mem_dtlb&PERF_MEM_TLB_OS)		std::cout << "\tOS fault handler\n";
	}

	void print () const {
		std::cout << *this;
	}

	friend std::ostream & operator << (std::ostream & os, const memory_data_cell_t & m) {
		os << "CPU: " << m.cpu << ", PID: " << m.pid << ", TID: " << m.tid <<
			"ADDR: " << m.addr << ", LATENCY: " << m.latency << ", DSRC: " << m.dsrc << ", TIME: " << m.time << '\n';
		return os;
	}
};

class memory_data_list_t {
public:
	std::vector<memory_data_cell_t> list;

	memory_data_list_t () {
		list.reserve(1000); // Speeds up doing preallocation
	}

	inline void add_cell (const uint32_t cpu, const int pid, const int tid, const uint64_t addr, const uint64_t latency, const uint64_t dsrc, const uint64_t time) {
		memory_data_cell_t cell(cpu, pid, tid, addr, latency, dsrc, time);
		list.push_back(cell);
	}

	inline bool is_empty () const {
		return list.empty();
	}

	inline void clear () {
		list.clear();
	}

	void print () const {
		std::cout << *this;
	}

	friend std::ostream & operator << (std::ostream & os, const memory_data_list_t & m) {
		for (const auto & l : m.list) {
			os << l;
		}
		return os;
	}
};

#endif
