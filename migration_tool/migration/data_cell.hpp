#pragma once
#ifndef __DATA_CELL__
#define __DATA_CELL__

#include "types_definition.hpp"

class data_cell_t {
public:
	cpu_t cpu;
	pid_t pid;
	tid_t tid;
	tim_t time;

	data_cell_t (const cpu_t cpu, const pid_t pid, const tid_t tid, const tim_t time) :
		cpu(cpu),
		pid(pid),
		tid(tid),
		time(time)
	{}

	inline void print () const {
		std::cout << *this << '\n';
	}

	friend std::ostream & operator << (std::ostream & os, const data_cell_t & d) {
		os << "CPU: " << d.cpu << ", PID: " << d.pid << ", TID: " << d.tid << ", TIME: " << d.time;
		return os;
	}
};

#endif /* end of include guard: __DATA_CELL__ */
