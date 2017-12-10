#include "system_struct.h"

// Declaraion of static variables
int NUM_OF_CPUS;
int NUM_OF_MEMORIES;
int CPUS_PER_MEMORY;
int** node_cpu_map;

// Gets info about number of CPUs, memory nodes and creates two maps (cpu to node and node to cpu)
int detect_system() {
	char filename[BUFSIZ];
	FILE *fff;
	int package;

	NUM_OF_CPUS = sysconf(_SC_NPROCESSORS_ONLN);

	// Experimental way to get NUM_OF_MEMORIES beforehand
	fff=fopen("/sys/devices/system/node/possible","r");
	if (fff==NULL)
		return -1;
	fscanf(fff,"0-%d",&NUM_OF_MEMORIES); // Should contain "0" or "0-last_node_id"
	NUM_OF_MEMORIES++; // Because "0" means 1 node, and in the other case it would be "last_node_id+1" nodes
	fclose(fff);

	CPUS_PER_MEMORY = NUM_OF_CPUS / NUM_OF_MEMORIES;

	node_cpu_map = (int**)malloc(NUM_OF_MEMORIES*sizeof(int*));
	for(int i=0;i<NUM_OF_MEMORIES;i++)
		node_cpu_map[i] = (int*)malloc(CPUS_PER_MEMORY*sizeof(int));

	int counters[CPUS_PER_MEMORY];
	memset(counters, 0, sizeof(counters));

	// For each CPU, reads topology file to get package (node) id
	for(int i=0;i<NUM_OF_CPUS;i++) {
		sprintf(filename,"/sys/devices/system/cpu/cpu%d/topology/physical_package_id",i);
		fff=fopen(filename,"r");
		if (fff==NULL) break;
		int dummy = fscanf(fff,"%d",&package);
		fclose(fff);

		if(dummy == 0)
			return -1; // Should not happen

		// Saves data to structure
		int index = counters[package];
		node_cpu_map[package][index] = i;

		counters[package]++;
	}

	//printf("Detected system: %d total CPUs, %d memory nodes, %d CPUs per node.\n", NUM_OF_CPUS, NUM_OF_MEMORIES, CPUS_PER_MEMORY);

	return 0;
}

