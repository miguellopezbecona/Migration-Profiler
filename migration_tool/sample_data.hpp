#pragma once
#ifndef __SAMPLE_DATA__
#define __SAMPLE_DATA__

#include <iostream>
#include <iomanip>

#include <cstdint>
#include <cstring>

#include <vector>

#ifdef JUST_PROFILE_ENERGY
#include "rapl/energy_data.hpp"
#endif

// For omitting usually useless columns while printing: IIP, TIME_E, TIME_R, MEM_OPS, DSRC
#define SIMPL_PRINT

class my_pebs_sample_t {
public:
	uint64_t iip;
	int      pid;
	int      tid;
	uint64_t time;
	uint64_t sample_addr;
	uint32_t cpu;
	uint64_t weight;
	uint64_t time_enabled;
	uint64_t time_running;
	uint64_t nr;
	uint64_t dsrc;
	uint64_t * values;

	#ifdef JUST_PROFILE_ENERGY
	std::vector<double> energies;
	#endif

	#ifdef JUST_PROFILE
	static size_t max_nr;
	static std::vector<char*> inst_subevent_names;

	static void add_subevent_name (const char * const name) {
		inst_subevent_names.push_back(strdup(name));
	}
	#endif

	my_pebs_sample_t () :
		energies(energy_data_t::NUM_RAPL_DOMAINS)
	{
		#ifdef JUST_PROFILE_ENERGY
		for (int i = 0; i < energy_data_t::NUM_RAPL_DOMAINS; i++) {
			energies[i] = -1.0;
		}
		#endif
		#ifdef JUST_PROFILE
		inst_subevent_names = std::vector<char*>();
		#endif
	}

	inline bool is_mem_sample () const {
		return nr == 1; // sample_addr != 0
	}

	#ifdef JUST_PROFILE
	void print (std::ostream & os) const {
		const bool mem_sample = is_mem_sample();
		const char type = mem_sample ? 'M' : 'I';

		#ifdef SIMPL_PRINT
		os << type << "," << pid << "," << tid << "," << cpu << "," << time << "," << sample_addr << "," << weight;
		#else
		os << "," << type << "," << ii p<< "," << pid << "," << tid << "," << cpu << "," << time << ","
			<< sample_addr << "," << weight << "," << time_enabled << "," << time_running << "," << dsrc;
		#endif

		if (mem_sample) {
			for (size_t i = 0; i < my_pebs_sample_t::max_nr; i++) { // No inst subevents
				os << ",0";
			}
			#ifndef SIMPL_PRINT
			os << "," << values[0]; // Mem ops
			#endif
		} else {
			for (long int i = nr-1; i >= 0; i--) { // Inst subevents
				os << "," << values[i];
			}
			#ifndef SIMPL_PRINT
			os << ",0"; // No MEM_OPS
			#endif
		}

		#ifdef JUST_PROFILE_ENERGY
		os.precision(3); os << std::fixed;
		for (int i = 0; i < energy_data_t::NUM_RAPL_DOMAINS; i++) {
			os << "," << energies[i];
		}
		os << std::defaultfloat;
		#endif

		os << '\n';
	}

	// static void print_header (FILE * const fp) {
	static void print_header (std::ostream & os) {
		#ifdef SIMPL_PRINT
		os << "TYPE,PID,TID,CPU,TIME,SAMPLE_ADDR,WEIGHT";
		#else
		os << "TYPE,IIP,PID,TID,CPU,TIME,SAMPLE_ADDR,WEIGHT,TIME_E,TIME_R,DSRC";
		#endif

		for (char * const & e_name : my_pebs_sample_t::inst_subevent_names) {
			os << "," << e_name;
		}

		#ifndef SIMPL_PRINT
		os << ",MEM_OPS";
		#endif

		#ifndef JUST_PROFILE_ENERGY
		os << '\n';
		#else
		for (char * const & d_name : energy_data_t::rapl_domain_names) {
			os << "," << d_name;
		}
		os << '\n';
		#endif
	}
	#endif

#ifdef JUST_PROFILE
	#ifdef JUST_PROFILE_ENERGY
	void add_energy_data () {
		const auto node = system_struct_t::cpu_node_map[cpu];
		const auto vals = ed.get_curr_vals(); // Energy values
		// const double ** vals = ed.get_curr_vals(); // Energy values

		// Copies energy values from the associated node to CPU sample
		memcpy(&energies[0], vals[node], energy_data_t::NUM_RAPL_DOMAINS * sizeof(double));
	}
	#endif

	friend std::ostream & operator << (std::ostream & os, const my_pebs_sample_t & s) {
		const bool mem_sample = s.is_mem_sample();
		const char type = mem_sample ? 'M' : 'I';

		#ifdef SIMPL_PRINT
			os << type << "," << s.pid << "," << s.tid << "," << s.cpu << "," << s.time << "," << s.sample_addr << "," << s.weight;
		#else
			os << type << "," << s.iip << "," << s.pid << "," << s.tid << "," << s.cpu << "," << s.time << "," <<
				s.sample_addr << "," << s.weight << "," << s.time_enabled << "," << s.time_running << "," << s.dsrc;
		#endif

		if (mem_sample) {
			for (size_t i = 0; i < my_pebs_sample_t::max_nr; i++) { // No inst subevents
				os << ",0";
			}
			#ifndef SIMPL_PRINT
			os << s.values[0];
			#endif
		} else {
			for (long int i = s.nr - 1; i >= 0; i--) { // Inst subevents
				os << "," << s.values[i];
			}
			#ifndef SIMPL_PRINT
			os << ",0"; // No MEM_OPS
			#endif
		}

		#ifdef JUST_PROFILE_ENERGY

		os.precision(2); os << std::fixed;
		for (int i = 0; i < energy_data_t::NUM_RAPL_DOMAINS; i++) {
			os << "," << s.energies[i];
		}
		os << std::defaultfloat;

		#endif

	    return os;
	}
#endif
};

#endif
