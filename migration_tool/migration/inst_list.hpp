#pragma once
#ifndef __INST_LIST__
#define __INST_LIST__

#include <iostream>
#include <cinttypes>

#include <vector> // std::vector

#include "system_struct.hpp" // NUM_OF_CPUS

class inst_data_cell_t {
public:
	uint32_t cpu;
	int pid;
	int tid;
	uint64_t inst;
	uint64_t req_dr;
	uint64_t time;

	inst_data_cell_t () {};

	inst_data_cell_t (const uint32_t cpu, const int pid, const int tid, const uint64_t inst, const uint64_t req_dr, const uint64_t time) :
		cpu(cpu),
		pid(pid),
		tid(tid),
		inst(inst),
		req_dr(req_dr),
		time(time)
	{};

	void print() const {
		std::cout << *this << '\n';
	}

	inline friend std::ostream & operator << (std::ostream & os, const inst_data_cell_t & i) {
		os << "CPU: " << i.cpu << ", PID: " << i.pid << ", TID: " << i.tid << ", INST: " << i.inst << ", REQ_DR: " << i.req_dr << ", TIME: " << i.time;
		return os;
	}
};

class inst_data_list_t {
public:
	std::vector<inst_data_cell_t> list;

	inst_data_list_t () {
		list.reserve(1000); // Speeds up doing preallocation
	}

	void inline add_cell (const uint32_t cpu, const int pid, const int tid, const uint64_t inst, const uint64_t req_dr, const uint64_t time) {
		inst_data_cell_t cell(cpu, pid, tid, inst, req_dr, time);
		list.push_back(cell);
	}

	void inline clear () {
		list.clear();
	}

	void print() const {
		std::cout << *this;
	}

	friend std::ostream & operator << (std::ostream & os, const inst_data_list_t & i) {
		for (const auto & l : i.list) {
			os << l << '\n';
		}
		return os;
	}

	int create_increments () {
		if (list.empty()) return -1;

		std::vector<uint64_t> last_inst(system_struct_t::NUM_OF_CPUS, 0);
		std::vector<uint64_t> last_req_dr(system_struct_t::NUM_OF_CPUS, 0);
		std::vector<uint64_t> last_time(system_struct_t::NUM_OF_CPUS, 0);
		uint64_t temp_inst;
		uint64_t temp_req_dr;
		uint64_t temp_time;

		//now let us suppose data are ordered by time FOR EACH CPU
		for (auto & cell : list ) {
			temp_inst = cell.inst;
			temp_req_dr = cell.req_dr;
			temp_time = cell.time;
			if (last_time[cell.cpu] == 0) { // it is the first datum of the cpu, set to zero, should be taken care when building the roofline
				cell.inst = 0;
				cell.req_dr = 0;
				cell.time = 0;
			} else {
				cell.inst -= last_inst[cell.cpu];
				cell.req_dr -= last_req_dr[cell.cpu];
				cell.time -= last_time[cell.cpu];
			}
			last_inst[cell.cpu] = temp_inst;
			last_req_dr[cell.cpu] = temp_req_dr;
			last_time[cell.cpu] = temp_time;
		}

		return 0;
	}
};

#endif
