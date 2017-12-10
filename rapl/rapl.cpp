#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

#include <numeric> // accumulate

#include "energy_data.h"

// No incremental data gathering
#define ONE_MEASURE

//#define PRINT_OUTPUT
#define PRINT_TO_FILE

// Copy paste from utils
double get_median_from_list(vector<double> l){
	size_t size = l.size();

	sort(l.begin(), l.end()); // Getting median requires sorting

	if (size % 2 == 0)
		return (l[size / 2 - 1] + l[size / 2]) / 2;
	else 
		return l[size / 2];
}

/////

#ifdef ONE_MEASURE
struct timeval t_beg, t_end;
#endif

energy_data_t ed;

// [TODO]: it could be interesting dumping all the values for all the domains, instead of just focusing on "cores"
typedef vector<double> energies;
vector<energies> cons_per_node;

void print_data(vector<energies> data){
	const char* data_filename = "data.csv";
	FILE* fp = fopen(data_filename, "w");
	if(fp == NULL){
		printf("Error opening file %s to log data.\n", data_filename);
		return;
	}

	fprintf(fp, "val,node,it\n"); // Header

	size_t v_size = data[0].size();
	for(int n=0;n<system_struct_t::NUM_OF_MEMORIES;n++){
		for(size_t it=0;it<v_size;it++)
			fprintf(fp, "%.3f,%d,%lu\n", data[n][it], n, it);
	}
	fclose(fp);

	// Print additional statistics in other file
	const char* metadata_filename = "metadata.csv";
	fp = fopen(metadata_filename, "w");
	if(fp == NULL){
		printf("Error opening file %s to log data.\n", metadata_filename);
		return;
	}

	fprintf(fp, "n,v_size,min,mean,max,median\n"); // Header
	for(int n=0;n<system_struct_t::NUM_OF_MEMORIES;n++){
		vector<double> v = data[n];

		double min = *(min_element(v.begin(), v.end()));
		double mean = accumulate(v.begin(), v.end(), 0.0) / v_size;
		double max = *(max_element(v.begin(), v.end()));
		double median = get_median_from_list(v);

		fprintf(fp, "%d,%lu,%.3f,%.3f,%.3f,%.3f\n",n,v_size,min,mean,max,median);
	}
	fclose(fp);
}

static void clean_end(int n) {
	#ifdef ONE_MEASURE
	ed.read_buffer();
	gettimeofday(&t_end, NULL);
	double elapsed_time = (t_end.tv_sec - t_beg.tv_sec + (t_end.tv_usec - t_beg.tv_usec)/1.e6);
	printf("Elapsed time: %.2f seconds\n", elapsed_time);

	// Gets all final values (all nodes, all domains) and prints them, along with mean consumption per second (all for total consumption per node)
	double total_sys_cons = 0.0;

	double** all_vals = ed.get_curr_vals();
	for(int n=0;n<system_struct_t::NUM_OF_MEMORIES;n++) {
		double total_node_cons = 0.0;
		printf("Node %d:\n",n);

		for(int d=0;d<energy_data_t::NUM_RAPL_DOMAINS;d++){
			total_node_cons += all_vals[n][d];
			printf("\tEnergy consumed (%s): %.2f Joules. Mean consumption per second: %.2f J/s\n", energy_data_t::rapl_domain_names[d], all_vals[n][d], all_vals[n][d] / elapsed_time);
		}
		total_sys_cons += total_node_cons;
		printf("\tTotal node consumption: %.2f Joules. Mean consumption: %.2f J/s\n\n", total_node_cons, total_node_cons / elapsed_time);
	}
	printf("Total SYSTEM consumption: %.2f Joules. Mean consumption: %.2f J/s\n\n", total_sys_cons, total_sys_cons / elapsed_time);
	#endif

	ed.close_buffers();

	#if defined(PRINT_TO_FILE) && ! defined(ONE_MEASURE)
	print_data(cons_per_node);
	#endif

	for(int i=0;i<system_struct_t::NUM_OF_MEMORIES;i++)
		cons_per_node[i].clear();
	cons_per_node.clear();

	exit(0);
}

int main(int argc, char **argv) {
	// Sets up handler for some signals for a clean t_end
	signal(SIGINT, clean_end);
	system_struct_t::detect_system();
	int ret = ed.prepare_energy_data();
	if(ret != 0)
		exit(ret);

	cons_per_node.resize(system_struct_t::NUM_OF_MEMORIES);

	#ifndef ONE_MEASURE
	int sleep_time = 1;
	while(1){
		sleep(sleep_time);
		ed.read_buffer();
		
		// We store core consumption for each node in our internal structure
		vector<double> data = ed.get_curr_vals_from_domain("cores");
		for(int i=0;i<system_struct_t::NUM_OF_MEMORIES;i++)
			cons_per_node[i].push_back(data[i]);

		#ifdef PRINT_OUTPUT
		ed.print_curr_vals();
		#endif
	}
	#else
	gettimeofday(&t_beg, NULL);
	pause();
	#endif

	return 0;
}
