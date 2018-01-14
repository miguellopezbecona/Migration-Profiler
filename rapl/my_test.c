#define _GNU_SOURCE 1
#include <stdio.h>
#include <unistd.h> // optarg, getting number of CPUs...
#include <sys/syscall.h> // gettid
#include <ctype.h> // isprint

#include <sched.h> // CPU affinity stuff
#include <errno.h> // Affinity error's constants
#include <omp.h> // OpenMP
#include <numa.h> // numa_alloc_onnode

#include "system_struct.h"


// Default macro values
#define DEFAULT_MAIN_ITERS 1000
#define DEFAULT_ELEMS_ITER 1
#define DEFAULT_REMOTE_READS 0
#define DEFAULT_NUMBER_OPS 1
#define DEFAULT_NUM_THREADS 1
#define DEFAULT_LOCAL_NODE 0
#define DEFAULT_REMOTE_NODE 1

#define CACHE_LINE_SIZE 64
#define ELEMS_PER_CACHE CACHE_LINE_SIZE / sizeof(float)

#define OUTPUT
//#define DOGETPID

// Main data
unsigned long array_basic_size; // array_total_size / num_th
unsigned long array_total_size;
float* local_array;
float* remote_array;

// Options
unsigned long int main_iters;
unsigned int elems_iter;
unsigned int remote_reads;
unsigned int ops;
unsigned int num_th;
unsigned char *selected_cpus;
unsigned char local_node;
unsigned char remote_node;

// Predeclarations
void print_selected_cpus();
void print_params();
void usage(char **argv);
void set_affinity_error();

/*** Auxiliar functions ***/
void print_selected_cpus(){
	printf("Selected CPUs:");

	int i;
	for(i=0;i<num_th;i++)
		printf(" %d", selected_cpus[i]);

	printf("\n");
}

void print_params(){
	//printf("Read iterations: %lu\nElements read per iteration: %d\nRemote elements read per iteration: %d\nNumber of floating operations per iteration: %d\nNumber of threads: %d\nArray size: %lu\nLocal node: %d\nRemote node: %d\n\n",main_iters,elems_iter,remote_reads,ops,num_th,array_total_size, local_node, remote_node);
	printf("R: %lu\nD: %d\nR: %d\nO: %d\nThs: %d\nArray size: %lu\nLocal node: %d\nRemote node: %d\n\n",main_iters,elems_iter,remote_reads,ops,num_th,array_total_size, local_node, remote_node);
}

void usage(char **argv) {
	printf("Usage: %s [-lread_iterations] [-delements_read_per_iteration] [-rremote_elements_read_per_iteration] [-ooperations_per_iteration] [-tnumber_of_threads] [-mlocal_node] [-Mremote_node]\n\n", argv[0]);
}

void set_affinity_error(){
	switch(errno){
		case EFAULT:
			printf("Error settting affinity: A supplied memory address was invalid\n");
			break;
		case EINVAL:
			printf("Error settting affinity: The affinity bitmask mask contains no processors that are physically on the system, or cpusetsize is smaller than the size of the affinity mask used by the kernel\n");
			break;
		case EPERM:
			printf("Error settting affinity: The calling process does not have appropriate privileges\n");
			break;
		case ESRCH:
			printf("Error settting affinity: The process whose ID is pid could not be found\n");
			break;
	}
}


/*** Main functions ***/

// Allocates memory and does other stuff
void data_initialization(){
	int th, offset;

	local_array = (float*)numa_alloc_onnode(array_total_size*sizeof(float), local_node);
	remote_array = (float*)numa_alloc_onnode(array_total_size*sizeof(float), remote_node);

	// Random initialization
	for(th=0;th<num_th;th++){
		offset = th*array_basic_size;

/*		// Can consume a lot of time and it's not necessary
		int i;
		for(i=0;i<array_basic_size;i++){
			local_array[offset+i] = (rand()%10)*1.0;
			remote_array[offset+i] = (rand()%10)*1.0;
		}
*/

		#ifdef OUTPUT
		// Gets the memory range where each thread will work in
		printf("local_array[%d] from %lx to %lx\nremote_array[%d] from %lx to %lx\n\n",
			th,(long unsigned int)&local_array[offset],(long unsigned int)&local_array[offset+array_basic_size],
			th,(long unsigned int)&remote_array[offset],(long unsigned int)&remote_array[offset+array_basic_size]
		);
		#endif
	}
}

// The kernel operation in the parallel zone. Defined as inline to reduce call overhead
static inline void operation(pid_t my_ompid){
	int i,n,r,o;
	int offset = my_ompid*array_basic_size; // Different work zone for each thread
	
	//float data_read;
	
	for(i=0; i<main_iters; i++){ // How many main iterations will we do?
		// We could multiply i by ELEMS_PER_CACHE to avoid being in the same cache line
		int index = offset + i;
	
		// This version avoids doing a lot of multiplications in inner loop. Compare with the next multiline comment
		int limit = elems_iter*ELEMS_PER_CACHE;
		for(n=0; n<limit; n+=ELEMS_PER_CACHE) // How many items will we process? (not in same cache line)
			local_array[n] = local_array[index+n];
			//data_read = local_array[index+n]; // First I tried just doing reads, but it didn't affect energy consumption

/*
		for(n=0; n<elems_iter; n++)
			data_read = local_array[index+n*ELEMS_PER_CACHE]; // Not in same cache line
*/

		limit = remote_reads*ELEMS_PER_CACHE;
		for(r=0; r<limit; r+=ELEMS_PER_CACHE) // How many remote items will we read/write?
			remote_array[r] = remote_array[index+r]; // Not in same cache line
			//data_read = remote_array[index+r]; // Not in same cache line

		for(o=0; o<ops; o++) // How many float operations per iteration?
			local_array[index+1] = local_array[index] * 1.42;
	}
}

void set_options_from_parameters(int argc, char** argv){
	char c;

	// Parses argv with getopt
	while ((c = getopt (argc, argv, "i:n:r:o:t:m:M:")) != -1){
		switch (c) {
			case 'i': // Main iterations
				main_iters = atol(optarg);
				break;
			case 'n': // Number of elements read/written per iteration
				elems_iter = atoi(optarg);
				break;
			case 'r': // Remote reads/writes per iteration
				remote_reads = atoi(optarg);
				break;
			case 'o': // Number of floating operations per iteration
				ops = atoi(optarg);
				break;
			case 't': // Number of threads
				num_th = atoi(optarg);
				
				// No more threads than CPUs per memory because they will all belong to the same node
				if(num_th > CPUS_PER_MEMORY)
					num_th = CPUS_PER_MEMORY;
				break;
			case 'm': // Local memory node
				local_node = atoi(optarg);
				break;
			case 'M': // Remote memory node
				remote_node = atoi(optarg);
				break;
			case '?': // Default
				if (isprint(optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
				usage(argv);
				exit(1);
		}
	}
}

void pick_cpus(){
	selected_cpus = (unsigned char*)calloc(num_th, sizeof(unsigned char));
	cpu_set_t aff;
	if(sched_getaffinity(0,sizeof(cpu_set_t),&aff))
		set_affinity_error();

	// Gets available CPU IDs from loca_node to pin the threads
	int i, picked_cpus = 0;
	for(i=0;i<CPUS_PER_MEMORY && i != num_th;i++) {
		int current_cpu = node_cpu_map[local_node][i]; // Working with node 0
		if(CPU_ISSET(current_cpu, &aff)){
			selected_cpus[picked_cpus] = current_cpu;
			picked_cpus++;
		}
	}
	
	// Lowers number of threads if less were picked due to numactl or stuff
	if(picked_cpus < num_th)
		num_th = picked_cpus;
}

void calculate_array_sizes(){
	// This way we avoid being out of bounds
	int max_p = elems_iter;
	if(remote_reads > max_p)
		max_p = remote_reads;
	array_basic_size = main_iters + max_p*ELEMS_PER_CACHE;

	array_total_size = array_basic_size*num_th; // In this case, the whole array would be proportional to the number of threads
	// [TOTHINK]: another option would be maintain the same array size, but reducing the number of reads per thread, or we could make the threads read each other's values
}

int main(int argc, char *argv[]){
	detect_system();

	// Set defaults
	main_iters = DEFAULT_MAIN_ITERS;
	elems_iter = DEFAULT_ELEMS_ITER;
	remote_reads = DEFAULT_REMOTE_READS;
	ops = DEFAULT_NUMBER_OPS;
	num_th = DEFAULT_NUM_THREADS;
	local_node = DEFAULT_LOCAL_NODE;
	remote_node = DEFAULT_REMOTE_NODE;

	set_options_from_parameters(argc, argv);
	
	// Sanity checks
	if(local_node >= NUM_OF_MEMORIES)
		local_node = 0;
	if(remote_node >= NUM_OF_MEMORIES)
		remote_node = NUM_OF_MEMORIES - 1; // Maximum
	
	pick_cpus();

	calculate_array_sizes();

	#ifdef OUTPUT
	// Just printings
	print_params();
	print_selected_cpus();
	#endif
	#ifdef DOGETPID
	printf("PID: %d\n", getpid()); // May be useful sometimes
	#endif

	// Sets number of threads to use
	omp_set_num_threads(num_th);
	
	data_initialization();

	// Parallel zone
	#pragma omp parallel
	{
		int tid = syscall(SYS_gettid);
		pid_t ompid = omp_get_thread_num(); // From 0 to num_th
		unsigned char my_cpu = selected_cpus[ompid]; // Picks selected CPU with omp index

		// Pins thread to CPU
		cpu_set_t my_affinity;
		CPU_ZERO(&my_affinity);
		CPU_SET(my_cpu, &my_affinity);
		if(sched_setaffinity(0,sizeof(cpu_set_t),&my_affinity))
			set_affinity_error();

		#ifdef OUTPUT
		printf("I am thread (%d,%d) and I got CPU %u\n", ompid, tid, my_cpu);
		#endif

		operation(ompid); // Kernel

		#ifdef OUTPUT
		printf("I am thread (%d,%d), pinned in CPU %d and I just finished\n", ompid, tid, my_cpu);
		#endif
	}

	// Frees resources and end
	int i;
	for(i=0;i<NUM_OF_MEMORIES;i++)
		free(node_cpu_map[i]);
	free(node_cpu_map);
	free(selected_cpus);
	numa_free(local_array, array_total_size*sizeof(float));
	numa_free(remote_array, array_total_size*sizeof(float));
	return 0;
}
