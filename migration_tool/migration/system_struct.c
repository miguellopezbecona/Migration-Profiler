#include "system_struct.h"

int system_struct_t::NUM_OF_CPUS;
int system_struct_t::NUM_OF_MEMORIES;
int system_struct_t::CPUS_PER_MEMORY;

// To know where each CPU is (in terms of memory node)
int* system_struct_t::cpu_node_map; // cpu_node_map[cpu] = node
vector<int> system_struct_t::node_cpu_map[MAX_PACKAGES]; // cpu_node_map[node] = list(cpus)

// To know where each TID is (in terms of CPUS)
map<pid_t, int> system_struct_t::tid_cpu_map; // input: tid, output: core
int* system_struct_t::cpu_tid_map; // input: core, output, tid

void set_affinity_error(pid_t tid);

// Gets info about number of CPUs, memory nodes and creates two maps (cpu to node and node to cpu)
int system_struct_t::detect_system() {
	char filename[BUFSIZ];
	FILE *fff;
	int package;

	NUM_OF_CPUS = sysconf(_SC_NPROCESSORS_ONLN);
	cpu_node_map = (int*)malloc(NUM_OF_CPUS*sizeof(int));
	cpu_tid_map = (int*)malloc(NUM_OF_CPUS*sizeof(int));

	memset(cpu_tid_map, FREE_CORE, NUM_OF_CPUS*sizeof(int)); // All CPUs are free at the beginning

	// For each CPU, reads topology file to get package (node) id
	for(int i=0;i<NUM_OF_CPUS;i++) {
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
	CPUS_PER_MEMORY = node_cpu_map[0].size();

	// Gets real number of packages (position of first empty vector)
	for(int i=0;i<MAX_PACKAGES;i++){
		if(node_cpu_map[i].empty()){
			NUM_OF_MEMORIES = i;
			break;
		}
	}

	printf("Detected system: %d total CPUs, %d memory nodes, %d CPUs per node.\n", NUM_OF_CPUS, NUM_OF_MEMORIES, CPUS_PER_MEMORY);

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
	int position = rand() % CPUS_PER_MEMORY;
	return node_cpu_map[cell][position];
}

int system_struct_t::get_ordered_cpu_from_node(int cell, int num){
	if(num >= CPUS_PER_MEMORY || num < 0)
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

int system_struct_t::set_tid_cpu(pid_t tid, int cpu){
	tid_cpu_map[tid] = cpu;
	cpu_tid_map[cpu] = tid;

	pin_thread_to_cpu(tid, cpu);
}

void system_struct_t::remove_tid(pid_t tid){
	tid_cpu_map.erase(tid);

	for(int i=0; i<NUM_OF_CPUS; i++){
		if(cpu_tid_map[i] == tid) {
			cpu_tid_map[i] = FREE_CORE;
			break;
		}
	}
}

bool system_struct_t::is_cpu_free(int cpu){
	return cpu_tid_map[cpu] == FREE_CORE;
}


/*** CPU-pin/free methods ***/
int system_struct_t::pin_thread_to_cpu(pid_t tid, int cpu) {
	cpu_set_t affinity;
	sched_getaffinity(tid, sizeof(cpu_set_t), &affinity);
	int ret = 0;

	CPU_ZERO(&affinity);
	CPU_SET(cpu, &affinity);

	if((ret = sched_setaffinity(tid, sizeof(cpu_set_t), &affinity))){
		set_affinity_error(tid);
		return errno;
	}

	return ret;
}

// Probably will need some from freeing TIDs or CPUs

// Auxiliar function
void set_affinity_error(pid_t tid){
	switch(errno){
		case EFAULT:
			printf("Error setting affinity: A supplied memory address was invalid\n");
			break;
		case EINVAL:
			printf("Error setting affinity: The affinity bitmask mask contains no processors that are physically on the system, or cpusetsize is smaller than the size of the affinity mask used by the kernel\n");
			break;
		case EPERM:
			printf("Error setting affinity: The calling process does not have appropriate privileges\n");
			break;
		case ESRCH: // When this happens, it's practically unavoidable
			//printf("Error setting affinity: The process whose ID is %d could not be found\n", tid);
			break;
	}
}
