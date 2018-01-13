#include "system_struct.hpp"

// Declaration of static variables
int system_struct_t::NUM_OF_CPUS;
int system_struct_t::NUM_OF_MEMORIES;
int system_struct_t::CPUS_PER_MEMORY;
int* system_struct_t::cpu_node_map;
vector<int> system_struct_t::node_cpu_map[MAX_PACKAGES];

// Gets info about number of CPUs, memory nodes and creates two maps (cpu to node and node to cpu)
int system_struct_t::detect_system() {
	char filename[BUFSIZ];
	FILE *fff;
	int package;

	NUM_OF_CPUS = sysconf(_SC_NPROCESSORS_ONLN);
	cpu_node_map = (int*)malloc(NUM_OF_CPUS*sizeof(int));

	// For each CPU, reads topology file to get package (node) id
	for(int i=0;i<NUM_OF_CPUS;i++) {
		sprintf(filename,"/sys/devices/system/cpu/cpu%d/topology/physical_package_id",i);
		fff=fopen(filename,"r");
		if (fff==NULL) break;
		int dummy = fscanf(fff,"%d",&package);
		fclose(fff);

		if(dummy == 0)
			return -1;

		// Saves data to structures
		cpu_node_map[i] = package;
		node_cpu_map[package].push_back(i);
	}

	// Size from first node
	CPUS_PER_MEMORY = node_cpu_map[0].size();

	// Gets real number of packages (position of first empty vector)
	for(int i=0;i<MAX_PACKAGES;i++){
		if(node_cpu_map[i].empty()){
			NUM_OF_MEMORIES = i;
			break;
		}
	}

	//printf("Detected system: %d total CPUs, %d memory nodes, %d CPUs per node.\n", NUM_OF_CPUS, NUM_OF_MEMORIES, CPUS_PER_MEMORY);

	return 0;
}

