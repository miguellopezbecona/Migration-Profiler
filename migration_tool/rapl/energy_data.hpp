#pragma once
#ifndef __ENERGY_DATA__
#define __ENERGY_DATA__

#include <iostream>
#include <string.h>
#include <dirent.h>

#include <vector>
#include <algorithm> // sort, min_element and max_element

#include "migration/system_struct.hpp"
#include "perfmon/perf_event.h"

class energy_data_t {
public:
	static std::vector<char*> rapl_domain_names;
	static int NUM_RAPL_DOMAINS;

	double** base_vals; // For base increments
	double** prev_vals; // For increments
	double** curr_vals;
	char** units;

	int** fd;
	double* scale;

private:
	static bool comparison_func (const char *c1, const char *c2) {
		return strcmp(c1, c2) < 0;
	}

public:
	energy_data_t () :
		base_vals(nullptr),
		prev_vals(nullptr),
		curr_vals(nullptr),
		units(nullptr),
		fd(nullptr),
		scale(nullptr)
	{}

	~energy_data_t () {
		if (base_vals == nullptr) {
			return;
		}

		for (int i = 0; i < system_struct_t::NUM_OF_MEMORIES; i++) {
			delete[] base_vals[i];
			delete[] prev_vals[i];
			delete[] curr_vals[i];
			delete[] fd[i];
		}

		for (int i = 0; i < NUM_RAPL_DOMAINS; i++)
			delete[] units[i];

		delete[] base_vals;
		delete[] prev_vals;
		delete[] curr_vals;
		delete[] units;
		delete[] fd;
		delete[] scale;
	}

	void allocate_data () {
		base_vals = new double* [system_struct_t::NUM_OF_MEMORIES];
		prev_vals = new double* [system_struct_t::NUM_OF_MEMORIES];
		curr_vals = new double* [system_struct_t::NUM_OF_MEMORIES];
		units = new char* [NUM_RAPL_DOMAINS];
		fd = new int* [system_struct_t::NUM_OF_MEMORIES];
		scale = new double[NUM_RAPL_DOMAINS];

		for (int i = 0; i < system_struct_t::NUM_OF_MEMORIES; i++) {
			base_vals[i] = new double[NUM_RAPL_DOMAINS]{};
			prev_vals[i] = new double[NUM_RAPL_DOMAINS]{};
			curr_vals[i] = new double[NUM_RAPL_DOMAINS];
			fd[i] = new int[NUM_RAPL_DOMAINS];
		}

		for (int i = 0; i < NUM_RAPL_DOMAINS; i++)
			units[i] = new char[8];
	}

	void detect_domains () {
		struct dirent * buffer = NULL;
		DIR * dir = NULL;
		const char * const folder = "/sys/bus/event_source/devices/power/events";

		if ( (dir = opendir(folder)) == NULL )
			return;

		while ( (buffer = readdir(dir)) != NULL ) {
			if (buffer->d_name[0] == '.') // No self-directory/parent
				continue;

			bool is_domain_name = true;

			// Equal to filename but without "energy-"
			char * no_energy = new char[strlen(buffer->d_name)-7];
			sscanf(buffer->d_name, "energy-%s", no_energy);

			// Domain names don't have a "." in their filename. All start with "energy-xxx", so we don't have to start in i=0
			for (size_t i = 10; buffer->d_name[i] != '\0'; i++) {
				if (buffer->d_name[i] == '.') {
					is_domain_name = false;
					break;
				}
			}

			if (is_domain_name)
				rapl_domain_names.push_back(strdup(no_energy));

			delete[] no_energy;
		}

		sort(rapl_domain_names.begin(), rapl_domain_names.end(), comparison_func); // Not necessary, but meh

		#ifdef INIT_VERBOSE
		std::cout << "RAPL domains detected: " << NUM_RAPL_DOMAINS << ". They are:";
		for (char* const & d_name : rapl_domain_names)
			std::cout << " " << d_name;
		std::std::cout << '\n';
		#endif


		closedir(dir);
	}

	int read_increments_file (const char * base_filename) {
		FILE * file = fopen(base_filename, "r");
		if (file == NULL) {
			std::cerr << "Base energy consumptions could not be read. Filename was: " << base_filename << '\n';
			return -1;
		}

		// For skipping header line
		char * buffer = new char[32];
		buffer = fgets(buffer, 32, file);

		int node;
		double val;

		while (true) {
			int ret = fscanf(file, "%d,%[^,],%lf", &node, buffer, &val);
			if (ret < 3)
				break;

			int col = get_domain_pos(buffer);
			base_vals[node][col] = val;
		}
		fclose(file);

		delete[] buffer;

		return 0;
	}

	int prepare_energy_data (const char * base_filename) {
		FILE * fff;
		int type;
		char filename[BUFSIZ]; // BUFSIZ is a system macro

		detect_domains();

		NUM_RAPL_DOMAINS = rapl_domain_names.size();

		allocate_data();

		fff = fopen("/sys/bus/event_source/devices/power/type","r");
		if (fff == NULL) {
			std::cerr << "\tNo perf_event rapl support found (requires Linux 3.14)" << '\n';
			std::cerr << "\tFalling back to raw msr support" << "\n\n";
			return -1;
		}
		int dummy = fscanf(fff, "%d", &type);
		fclose(fff);

		if (dummy != 1)
			return -1;

		int * config = new int[NUM_RAPL_DOMAINS];

		// Gets data to open counters
		for (int i = 0; i < NUM_RAPL_DOMAINS; i++) {
			sprintf(filename, "/sys/bus/event_source/devices/power/events/energy-%s", rapl_domain_names[i]);
			fff = fopen(filename, "r");

			if (fff != NULL) {
				dummy = fscanf(fff, "event=%x", &config[i]);
				fclose(fff);
			} else {
				std::cerr << "Error while opening config file for domain " << rapl_domain_names[i] <<
					".\nThis is probably due to that domain not being supported. It will be removed from the list." << '\n';
				rapl_domain_names.erase(rapl_domain_names.begin() + i);
				NUM_RAPL_DOMAINS--;
				continue;
			}

			sprintf(filename, "/sys/bus/event_source/devices/power/events/energy-%s.scale", rapl_domain_names[i]);
			fff = fopen(filename, "r");

			if (fff != NULL) {
				dummy = fscanf(fff, "%lf", &scale[i]);
				fclose(fff);
			}

			sprintf(filename, "/sys/bus/event_source/devices/power/events/energy-%s.unit", rapl_domain_names[i]);
			fff=fopen(filename, "r");

			if (fff != NULL) {
				dummy = fscanf(fff, "%s", units[i]);
				fclose(fff);
			}
		}

		read_increments_file(base_filename);

		// Opens counters
		for (int n = 0; n < system_struct_t::NUM_OF_MEMORIES; n++) {
			for (int d = 0; d < NUM_RAPL_DOMAINS; d++) {
				fd[n][d] = -1;

				struct perf_event_attr attr;
				memset(&attr, 0x0, sizeof(attr));
				attr.type = type;
				attr.config = config[d];
				if (config[d] == 0) continue;

				fd[n][d] = perf_event_open(&attr, -1, system_struct_t::node_cpu_map[n][0], -1, 0);
				if(fd[n][d] < 0) {
					std::cerr << "\tError code while opening buffer for CPU " << system_struct_t::node_cpu_map[n][0] << ", config " << config[d] << ": " << fd[n][d] << "\n\n";
					return -1;
				}
			}
		}

		delete[] config;

		return 0;
	}

	inline int get_domain_pos (const char * domain) {
		for (int i = 0; i < NUM_RAPL_DOMAINS; i++) {
			if (strcmp(rapl_domain_names[i], domain) == 0)
				return i;
		}
		return -1;
	}

	void read_buffer (const double secs) {
		long long value;

		for (int n = 0; n < system_struct_t::NUM_OF_MEMORIES; n++) {
			for (int d = 0; d < NUM_RAPL_DOMAINS; d++) {
				int dummy = read(fd[n][d],&value,8);

				// Can be uncommented to avoid a "unused-variable" warning, but I think it may affect performance
				/*
				if(dummy < 0)
					//return;
				*/
				double raw_value_scaled = ((double)value*scale[d]);
				#ifdef JUST_PROFILE_ENERGY
				curr_vals[n][d] = (raw_value_scaled - prev_vals[n][d]) / secs;
				#else
				curr_vals[n][d] = (raw_value_scaled - prev_vals[n][d]) / secs - base_vals[n][d]; // Normal and base increments
				#endif
				prev_vals[n][d] = raw_value_scaled;

				#ifndef JUST_PROFILE_ENERGY
				// If a negative value is raised due to varying base value, this is updated and the current is nullified
				if (curr_vals[n][d] < 0.0) {
					base_vals[n][d] += curr_vals[n][d];
					curr_vals[n][d] = 0.0;
				}
				#endif
			}
		}
	}

	inline double get_ratio_against_base (const double val, const int node, const char * domain) {
		const int col = get_domain_pos(domain);
		return val / base_vals[node][col];
	}

	void print_curr_vals () {
		for (int n = 0; n < system_struct_t::NUM_OF_MEMORIES; n++) {
			std::cout << "Node " << n << ":" << '\n';

			for (int d = 0; d < NUM_RAPL_DOMAINS; d++)
				std::cout << "\tEnergy consumed in " << rapl_domain_names[d] << ": " << curr_vals[n][d] << " " << units[d] << '\n';
		}
		std::cout << '\n';
	}

	// Some of them may not be used, but they can come handy in the future
	inline double get_curr_val (const int node, const char * domain) {
		const int pos = get_domain_pos(domain);

		return (pos < 0) ? 0.0 : curr_vals[node][pos];
	}

	inline std::vector<double> get_curr_vals_from_node (const int node) { // For all domains
		std::vector<double> v;

		for (int i = 0; i < NUM_RAPL_DOMAINS; i++)
			v.push_back(curr_vals[node][i]);

		return v;
	}

	std::vector<double> get_curr_vals_from_domain (const char * domain) { // For all nodes
		std::vector<double> v;
		int pos = get_domain_pos(domain);

		if (pos == -1)
			return v;

		for (int i = 0; i < system_struct_t::NUM_OF_MEMORIES; i++)
			v.push_back(curr_vals[i][pos]);

		return v;
	}

	inline double ** get_curr_vals () { // Everything
		return curr_vals;
	}

	void close_buffers () {
		for (int i = 0; i < system_struct_t::NUM_OF_MEMORIES; i++) {
			for (int j = 0; j < NUM_RAPL_DOMAINS; j++)
				close(fd[i][j]);
		}
	}
};

energy_data_t ed;

std::vector<char*> energy_data_t::rapl_domain_names;
int energy_data_t::NUM_RAPL_DOMAINS = 0;

#endif
