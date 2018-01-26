#include "system_struct.h"

// Declaraion of static variables
int system_struct_t::NUM_OF_CPUS;
int system_struct_t::NUM_OF_MEMORIES;
int system_struct_t::CPUS_PER_MEMORY;
int* system_struct_t::cpu_node_map;
vector<int> system_struct_t::node_cpu_map[MAX_PACKAGES];
map<pid_t, int> system_struct_t::tid_cpu_map;
int* system_struct_t::cpu_tid_map;
int** system_struct_t::node_distances;
map<pid_t, pid_t> system_struct_t::tid_pid_map;

vector<unsigned short> system_struct_t::ordered_cpus; // Ordered by distance nodes. Might be useful for genetic strategy


/*** Auxiliar functions ***/
void set_affinity_error(pid_t tid){
	switch(errno){
		case EFAULT:
			printf("Error setting affinity: A supplied memory address was invalid.\n");
			break;
		case EINVAL:
			printf("Error setting affinity: The affinity bitmask mask contains no processors that are physically on the system, or cpusetsize is smaller than the size of the affinity mask used by the kernel.\n");
			break;
		case EPERM:
			printf("Error setting affinity: The calling process does not have appropriate privileges for TID %d,\n", tid);
			break;
		case ESRCH: // When this happens, it's practically unavoidable
			//printf("Error setting affinity: The process whose ID is %d could not be found\n", tid);
			break;
	}
}

// Each line will result in a row in the distance matrix
void read_line_from_file(int node, int* array){
	char filename[64] = "\0";
	FILE *file = NULL;

	sprintf(filename, "/sys/devices/system/node/node%d/distance", node);
	file = fopen(filename, "r");

	if(file == NULL)
		return;

	for(int i=0;fscanf(file, "%d", &array[i]) == 1; i++);

	fclose(file);
}

bool are_all_nodes_processed(bool* processed){
	for(int i=0;i<system_struct_t::NUM_OF_MEMORIES;i++){
		if(!processed[i])
			return false;
	}
	return true;
}

/*** system_struct_t functions ***/

// Gets info about number of CPUs, memory nodes and creates two maps (cpu to node and node to cpu)
int system_struct_t::detect_system() {
	char filename[BUFSIZ];
	FILE *fff;
	int package;

	NUM_OF_CPUS = sysconf(_SC_NPROCESSORS_ONLN);
	cpu_node_map = (int*)malloc(NUM_OF_CPUS*sizeof(int));
	cpu_tid_map = (int*)malloc(NUM_OF_CPUS*sizeof(int));

	memset(cpu_tid_map, FREE_CPU, NUM_OF_CPUS*sizeof(int)); // All CPUs are free at the beginning

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

	// Initializes and builds node distance matrix
	node_distances = (int**)malloc(NUM_OF_MEMORIES*sizeof(int*));
	for(int i=0;i<NUM_OF_MEMORIES;i++){
		node_distances[i] = (int*)malloc(NUM_OF_MEMORIES*sizeof(int));
		read_line_from_file(i, node_distances[i]);
	}


	printf("Detected system: %d total CPUs, %d memory nodes, %d CPUs per node.\n", NUM_OF_CPUS, NUM_OF_MEMORIES, CPUS_PER_MEMORY);
	//print_node_distance_matrix();

	// We will calculate ordered_cpus (does not take into account hyperthreading, so a core won't be next to its HT partner)
	ordered_cpus.reserve(NUM_OF_CPUS);

	bool processed[NUM_OF_MEMORIES];
	memset(processed, false, NUM_OF_MEMORIES*sizeof(bool));

	// We begin with node 0
	int ref_node = 0;
	ordered_cpus.insert(end(ordered_cpus), begin(node_cpu_map[0]), end(node_cpu_map[0]));
	processed[0] = true;
	while(!are_all_nodes_processed(processed)){

		// We get the nearest not processed node to the current one
		int min_index = -1;
		int min_distance = 1e3;
		for(int i=0;i<NUM_OF_MEMORIES;i++){
			if(i == ref_node || processed[i])
				continue;

			if(node_distances[ref_node][i] < min_distance){
				min_distance = node_distances[ref_node][i];
				min_index = i;
			}
		}

		// We got the new mininum: mark as processed and concat its CPUs
		processed[min_index] = true;
		ordered_cpus.insert(end(ordered_cpus), begin(node_cpu_map[min_index]), end(node_cpu_map[min_index]));
		
		ref_node = min_index;

	}

	// Now we should have the CPUs ordered as we want
/*
	for(unsigned short c : ordered_cpus)
		printf("%d ", c);
	printf("\n");
*/

	return 0;
}

void system_struct_t::clean(){
	free(cpu_node_map);
	for(int i=0;i<NUM_OF_MEMORIES;i++){
		node_cpu_map[i].clear();
		free(node_distances[i]);
	}
	free(node_distances);
	tid_cpu_map.clear();
	free(cpu_tid_map);
	ordered_cpus.clear();
	tid_pid_map.clear();
}

/*** Node-CPU methods ***/
bool system_struct_t::is_in_same_memory_node(int cpu1, int cpu2){
	return cpu_node_map[cpu1] == cpu_node_map[cpu2];
}

int system_struct_t::get_cpu_memory_node(int cpu){
	return cpu_node_map[cpu];
}

int system_struct_t::get_random_cpu_in_node(int node){
	int position = rand() % CPUS_PER_MEMORY;
	return node_cpu_map[node][position];
}


/*** CPU-thread methods ***/
void system_struct_t::add_tid(pid_t tid, int cpu){
	if(tid_cpu_map.count(tid)) // We don't continue if the TID already exists in map
		return;

	if(is_cpu_free(cpu)){ // The thread is also pinned to the CPU if it is free
		set_tid_cpu(tid, cpu, true);
		return;
	}
	
	// If not, we search a free CPU on its same node
	int node = cpu_node_map[cpu];
	for(int const & other_cpu : node_cpu_map[node]){
		if(is_cpu_free(other_cpu)){
			set_tid_cpu(tid, other_cpu, true);
			return;
		}
	}

	// If we are here, we didn't found a free CPU in the memory node of the CPU where the sample was gotten. We will try with the rest
	// [TODO]: We could use the info from node_distances to continue the search from the nearest node to the initial one to the farthest one
	for(int n=0;n<NUM_OF_MEMORIES;n++){
		if(n == node)
			 continue; // Already checked

		for(int const & other_cpu : node_cpu_map[n]){
			if(is_cpu_free(other_cpu)){ // The thread is pinned to the CPU if it is free
				set_tid_cpu(tid, other_cpu, true);
				return;
			}
		}
	}

	set_tid_cpu(tid, cpu, false); // If there aren't any free CPUs, we assign the thread to the initial CPU without pinning it
}

int system_struct_t::get_cpu_from_tid(pid_t tid){
	return tid_cpu_map[tid];
}

int system_struct_t::get_tid_from_cpu(int cpu){
	return cpu_tid_map[cpu];
}

int system_struct_t::set_tid_cpu(pid_t tid, int cpu, bool do_pin){
	tid_cpu_map[tid] = cpu;

	if(!do_pin)
		return 0;
	
	cpu_tid_map[cpu] = tid;
	return pin_thread_to_cpu(tid, cpu);
}

void system_struct_t::remove_tid(pid_t tid, bool do_unpin){
	tid_pid_map.erase(tid);

	int cpu = tid_cpu_map[tid];
	tid_cpu_map.erase(tid);

	// Already finished threads don't need unpin
	if(do_unpin){
		cpu_tid_map[cpu] = FREE_CPU;
		unpin_thread(tid);
	}
}

bool system_struct_t::is_cpu_free(int cpu){
	return cpu_tid_map[cpu] == FREE_CPU;
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

int system_struct_t::unpin_thread(pid_t tid) {
	cpu_set_t affinity;
	sched_getaffinity(0, sizeof(cpu_set_t), &affinity); // Gets profiler's affinity (all CPUs)
	int ret = 0;

	// Gives profiler's affinity to freed thread
	if((ret = sched_setaffinity(tid, sizeof(cpu_set_t), &affinity))){
		set_affinity_error(tid);
		return errno;
	}

	return ret;
}

/*** Node distances methods ***/
int system_struct_t::get_node_distance(int node1, int node2){
	return node_distances[node1][node2];
}

void system_struct_t::print_node_distance_matrix(){
	for(int i=0;i<NUM_OF_MEMORIES;i++){
		for(int j=0;j<NUM_OF_MEMORIES;j++)
			printf("%d ", node_distances[i][j]);
		printf("\n");
	}	
}

/*** Other functions ***/
void system_struct_t::set_pid_to_tid(pid_t pid, pid_t tid){
	tid_pid_map[tid] = pid;
}

pid_t system_struct_t::get_pid_from_tid(pid_t tid){
	return tid_pid_map[tid];
}
