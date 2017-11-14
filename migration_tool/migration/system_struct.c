#include "system_struct.h"

int system_struct_t::NUM_OF_CORES;
int system_struct_t::NUM_OF_MEMORIES;
int system_struct_t::CORES_PER_MEMORY;

int* system_struct_t::cpu_node_map; // cpu_node_map[cpu] = node
vector<int> system_struct_t::node_cpu_map[MAX_PACKAGES]; // cpu_node_map[node] = list(cpus)

map<pid_t, int> system_struct_t::tid_cpu_map;
int* system_struct_t::cpu_tid_map;

// Gets info about number of cores, memory nodes and creates two maps (cpu to node and node to cpu)
int system_struct_t::detect_system() {
	char filename[BUFSIZ];
	FILE *fff;
	int package;

	NUM_OF_CORES = sysconf(_SC_NPROCESSORS_ONLN);
	cpu_node_map = (int*)malloc(NUM_OF_CORES*sizeof(int));
	cpu_tid_map = (int*)malloc(NUM_OF_CORES*sizeof(int));

	// For each CPU, reads topology file to get package (node) id
	for(int i=0;i<NUM_OF_CORES;i++) {
		sprintf(filename,"/sys/devices/system/cpu/cpu%d/topology/physical_package_id",i);
		fff=fopen(filename,"r");
		if (fff==NULL) break;
		fscanf(fff,"%d",&package);
		fclose(fff);

		// Saves data to structures
		cpu_node_map[i] = package;
		node_cpu_map[package].push_back(i);
	}

	// Size from first node
	CORES_PER_MEMORY = node_cpu_map[0].size();

	// Gets real number of packages (position of first empty vector)
	for(int i=0;i<MAX_PACKAGES;i++){
		if(node_cpu_map[i].empty()){
			NUM_OF_MEMORIES = i;
			break;
		}
	}

	printf("Detected system: %d total cores, %d memory nodes, %d cores per node.\n", NUM_OF_CORES, NUM_OF_MEMORIES, CORES_PER_MEMORY);

	return 0;
}

/*** Node-CPU methods ***/
bool system_struct_t::is_in_same_memory_cell(int cpu1, int cpu2){
	return cpu_node_map[cpu1] == cpu_node_map[cpu2];
}

int system_struct_t::get_cpu_memory_cell(int cpu){
	return cpu_node_map[cpu];
}

int system_struct_t::get_random_core_in_cell(int cell){
	int position = rand() % CORES_PER_MEMORY;
	return node_cpu_map[cell][position];
}

int system_struct_t::get_ordered_cpu_from_node(int cell, int num){
	if(num >= CORES_PER_MEMORY || num < 0)
		return -1;

	return node_cpu_map[cell][num];
}


/*** CPU-thread methods ***/
int system_struct_t::get_cpu_from_tid(pid_t tid){
	return tid_cpu_map[tid];
}

int system_struct_t::get_tid_from_cpu(int cpu){
	return cpu_tid_map[cpu];
}

void system_struct_t::set_tid_core(pid_t tid, int core){
	tid_cpu_map[tid] = core;
	cpu_tid_map[core] = tid;
}

void system_struct_t::remove_tid(pid_t tid){
	tid_cpu_map.erase(tid);

	for(int i=0; i<NUM_OF_CORES; i++){
		if(cpu_tid_map[i] == tid) {
			cpu_tid_map[i] = FREE_CORE;
			break;
		}
	}
}

bool system_struct_t::is_cpu_free(int cpu){
	return cpu_tid_map[cpu] == FREE_CORE;
}
