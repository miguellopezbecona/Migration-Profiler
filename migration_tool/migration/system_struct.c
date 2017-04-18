#include "system_struct.h"

int SYS_NUM_OF_CORES;
int SYS_NUM_OF_MEMORIES;
int SYS_CORES_PER_MEMORY;

int* cpu_node_map; // cpu_node_map[cpu] = node
vector<int> node_cpu_map[MAX_PACKAGES]; // cpu_node_map[node] = list(cpus)

map<int, int> tid_core_map;
int* core_tid_map;

// Gets info about number of cores, memory nodes and creates two maps (cpu to node and node to cpu)
int detect_system() {
	char filename[BUFSIZ];
	FILE *fff;
	int package;

	SYS_NUM_OF_CORES = sysconf(_SC_NPROCESSORS_ONLN);
	cpu_node_map = (int*)malloc(SYS_NUM_OF_CORES*sizeof(int));

	core_tid_map = (int*)malloc(SYS_NUM_OF_CORES*sizeof(int));

	// For each CPU, reads topology file to get package (node) id
	for(int i=0;i<SYS_NUM_OF_CORES;i++) {
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
	SYS_CORES_PER_MEMORY = node_cpu_map[0].size();

	// Gets real number of packages (position of first empty vector)
	for(int i=0;i<MAX_PACKAGES;i++){
		if(node_cpu_map[i].empty()){
			SYS_NUM_OF_MEMORIES = i;
			break;
		}
	}

	//printf("Detected system: %d total cores, %d memory nodes, %d cores per node.\n", SYS_NUM_OF_CORES, SYS_NUM_OF_MEMORIES, SYS_CORES_PER_MEMORY);

	return 0;
}

bool is_in_same_memory_cell(int cpu1, int cpu2){
	return cpu_node_map[cpu1] == cpu_node_map[cpu2];
}

int get_cpu_memory_cell(int cpu){
	return cpu_node_map[cpu];
}

int get_random_core_in_cell(int cell){
	int position = rand() % SYS_CORES_PER_MEMORY;
	return node_cpu_map[cell][position];
}

int get_tid_core(pid_t tid){
	return tid_core_map[tid];
}

void set_tid_core(pid_t tid, int core){
	tid_core_map[tid] = core;
	core_tid_map[core] = tid;
}

void remove_tid(pid_t tid){
	tid_core_map.erase(tid);
}
