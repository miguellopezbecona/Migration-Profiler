#pragma once

#include <string.h>
#include <dirent.h>

#include <vector>
#include <algorithm> // sort, min_element and max_element
using namespace std;

#include "../migration/system_struct.h"
#include "../perfmon/perf_event.h"

typedef struct energy_data {
	static vector<char*> rapl_domain_names;
	static int NUM_RAPL_DOMAINS;

	double** base_vals; // For baseincrements
	double** prev_vals; // For increments
	double** curr_vals;
	char** units;

	int** fd;
	double* scale;

	energy_data(){}
	~energy_data();
	void detect_domains();
	int read_increments_file(char* base_filename);
	void allocate_data();
	int prepare_energy_data(char* base_filename);

	int get_domain_pos(const char* domain);

	void read_buffer(double secs);
	void print_curr_vals();

	// Some of them may not be used, but they can come handy in the future
	double get_curr_val(int node, const char* domain);
	vector<double> get_curr_vals_from_node(int node); // For all domains
	vector<double> get_curr_vals_from_domain(const char* domain); // For all nodes

	double** get_curr_vals(); // Everything

	void close_buffers();
} energy_data_t;

extern energy_data_t ed;

