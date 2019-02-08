#pragma once
#ifndef __THREADS_TABLE__
#define __THREADS_TABLE__

#include <iostream>
#include <iomanip>

#include <vector>
#include <map>

#include "types_definition.hpp"
#include "data_cell.hpp"
#include "inst_list.hpp"
#include "memory_list.hpp"

template<typename _Tp>
class my_vec : public std::vector<_Tp> {
public:
	my_vec () {
		this->reserve(1000);
	}

	my_vec (const size_t capacity) {
		this->reserve(capacity);
	}
};

class threads_table_t {
private:
	std::map<tid_t, my_vec<data_cell_t>> 		thr_table;
	std::map<tid_t, my_vec<inst_data_cell_t>> 	ins_table;
	std::map<tid_t, my_vec<memory_data_cell_t>>	mem_table;
	std::map<cpu_t, std::set<pid_t>> 			cpu_table;

public:
	inline void add_data (const inst_data_cell_t & data) {
		ins_table[data.tid].push_back(data);
		thr_table[data.tid].push_back(data);
		cpu_table[data.cpu].insert(data.tid);
	}

	inline void add_data (const memory_data_cell_t & data) {
		mem_table[data.tid].push_back(data);
		thr_table[data.tid].push_back(data);
		cpu_table[data.cpu].insert(data.tid);
	}

	inline void clear () {
		thr_table.clear();
		ins_table.clear();
		mem_table.clear();
		cpu_table.clear();
	}

	inline void print () {
		std::cout << *this << '\n';
	}

	friend std::ostream & operator << (std::ostream & os, const threads_table_t & tt) {
		os << "Threads map: " << tt.thr_table.size() << " entries" << '\n';
		for (const auto & entry : tt.thr_table) {
			const auto & tid = entry.first;

			os << "\tTID: " << tid << ". ";
			const auto & ins_data_it = tt.ins_table.find(tid);
			if (ins_data_it != tt.ins_table.end()) {
				const auto & ins_data = ins_data_it->second;
				os << ins_data.size() << " instruction samples. ";
			}

			const auto & mem_data_it = tt.mem_table.find(tid);
			if (mem_data_it != tt.mem_table.end()) {
				const auto & mem_data = mem_data_it->second;
				os << mem_data.size() << " memory samples.";
			}
			os << '\n';
		}
		os << "CPU map: " << tt.cpu_table.size() << " entries" << '\n';
		for (const auto & cpu : tt.cpu_table) {
			os << "\tCPU " << std::setw(3) << cpu.first << " has TIDs: ";
			const auto & tids = cpu.second;
			for (const auto & tid : tids) {
				os << tid << " ";
			}
			os << '\n';
		}
		return os;
	}
};

#endif /* end of include guard: __THREADS_TABLE__ */
