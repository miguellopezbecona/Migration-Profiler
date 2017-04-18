#define _GNU_SOURCE 1
#include <stdio.h>
#include <unistd.h> // optarg, getting number of CPUs...
#include <sys/syscall.h> // gettid
#include <ctype.h> // isprint

#include <sched.h> // CPU affinity stuff
#include <errno.h> // Affinity error's constants
#include <omp.h> // OpenMP
#include <numa.h> // numa_alloc_onnode

// Default macro values
#define DEFAULT_BLOCKS 1
#define DEFAULT_ARRAY_SIZE 1000
#define DEFAULT_MEM_NODE 0
#define DEFAULT_REP 100
#define DEFAULT_NUMBER_OPS 1
#define DEFAULT_STRIDE 1

//#define OUTPUT

int max_cpus; // Detected by the system

// Main data
float *A, *B, *C;

// Options
int blocks_per_thread;
int array_basic_size; // Actual array size will be array_basic_size*num_th elements
int mem_node;
int rep;
int ops;
int stride;
int num_th;
unsigned char *selected_cpus;

// Predeclarations
void print_selected_cpus();
void print_params();
void usage(char **argv);
void set_affinity_error();


/*** Auxiliar functions ***/
void print_selected_cpus(){
	printf("Selected CPUS:");

	int i;
	for(i=0;i<num_th;i++)
		printf(" %d", selected_cpus[i]);

	printf("\n");
}

void print_params(){
	printf("Initialising data in %d memory node.\nBlocks per thread: %d.\nRepetitions: %d.\nArray size: %d.\nStride: %d.\nNumber of threads: %d.\n",mem_node,blocks_per_thread,rep,array_basic_size,stride,num_th);
}

void usage(char **argv) {
	printf("Usage: %s [-bblocks_per_thread] [-mmemory-cpu] [-rrepetitions] [-ooperations_per_iteration] [-sarray_basic_size] [-ccpus] [-tstride]\n\n", argv[0]);
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

// Allocates memory in a memory node and initializes arrays
void data_initialization(){
	int i, th, offset;

	// Memory allocation on memory node
	A = numa_alloc_onnode(array_basic_size*sizeof(float)*num_th, mem_node);
	B = numa_alloc_onnode(array_basic_size*sizeof(float)*num_th, mem_node);
	C = numa_alloc_onnode(array_basic_size*sizeof(float)*num_th, mem_node);

	// Random initialization
	for(th=0;th<num_th;th++){
		offset = th*array_basic_size;

		for(i=0;i<array_basic_size;i++){
			A[offset+i] = (rand()%10)*1.0;
			B[offset+i] = (rand()%10)*1.0;
			C[offset+i] = 0.0;
		}

		#ifdef OUTPUT
		// Gets the memory range where each thread will work in
		printf("ABC: A[%d] from %lx to %lx\nB[%d] from %lx to %lx\nC[%d] from %lx to %lx\n",
			th,(long unsigned int)&A[offset],(long unsigned int)&A[offset+array_basic_size],
			th,(long unsigned int)&B[offset],(long unsigned int)&B[offset+array_basic_size],
			th,(long unsigned int)&C[offset],(long unsigned int)&C[offset+array_basic_size]
		);
		#endif
	}
}

// The kernel operation in the parallel zone. Defined as inline to reduce call overhead
static inline void operation(pid_t my_ompid){
	int r,b,i,o;
	int chunk_id;
	int offset; // Different work zone for each thread
	int index;
	
	for(r=0; r<rep; r++){ // How many times does each thread repeat the work?
		for(b=0; b<blocks_per_thread; b++){ // How many chunks will the thread work in?
			chunk_id = (my_ompid + b) % num_th; // Cyclic chunk assignation
			offset = chunk_id*array_basic_size;
			for(i=0; i<array_basic_size; i+=stride){ // Advances through the vector using stride
				index = offset + i;
				for(o=0; o<ops; o++) // How many operations per iteration?
					C[index] = A[index] * B[index];
			}
		}
	}
}

void set_options_from_parameters(int argc, char** argv){
	char c;

	// Parses argv with getopt
	while ((c = getopt (argc, argv, "b:m:r:o:s:t:c:")) != -1){
		switch (c) {
			case 'b': // How many consecutive chunks will every thread work in
				blocks_per_thread = atoi(optarg);
				break;
			case 'm': // Memory node to allocate data in
				mem_node = atoi(optarg);
				break;
			case 'r': // Number of repetitions
				rep = atoi(optarg);
				break;
			case 'o': // Number of floating operations per iteration
				ops = atoi(optarg);
				break;
			case 't': // Stride
				stride = atoi(optarg);
				break;
			case 's': // Array size
				array_basic_size = atol(optarg);
				break;
			case 'c': // Binary vector of CPUs to be used (0 -> not used, 1 -> used). For example, 0011 means we only select CPUs 2 and 3
				if(strlen(optarg) != max_cpus){
					printf("If you use -c option, you have to specify %d cpus\n", max_cpus);
					exit(-1);
				}

				num_th = 0;

				// We loop over the value to get which CPUs will we use: the "1"s in the string
				int i;
				for(i=0;i<max_cpus;i++){
					if (optarg[i] == '1'){ // CPU selected: one more thread to create, let's get the index (CPU ID)
						selected_cpus[num_th] = i;
						num_th++;
					}
				}

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

int main(int argc, char *argv[]){
	max_cpus = sysconf(_SC_NPROCESSORS_ONLN);
	selected_cpus = (unsigned char*)calloc(max_cpus, sizeof(unsigned char));

	// Set defaults
	blocks_per_thread = DEFAULT_BLOCKS;
	mem_node = DEFAULT_MEM_NODE;
	rep = DEFAULT_REP;
	ops = DEFAULT_NUMBER_OPS;
	array_basic_size = DEFAULT_ARRAY_SIZE;
	stride = DEFAULT_STRIDE;
	num_th = 0;

	// By default, all available CPUs will be used
	cpu_set_t aff;
	if(sched_getaffinity(0,sizeof(cpu_set_t),&aff))
		set_affinity_error();

	int i;
	for(i=0;i<max_cpus;i++) {
		if(CPU_ISSET(i, &aff)){ // Gets available CPU IDs (compatible with numactl)
			selected_cpus[num_th] = i;
			num_th++;
		}
	}

	set_options_from_parameters(argc, argv);

	// Reallocs correct size of selected_cpus
	selected_cpus = (unsigned char*)realloc(selected_cpus, num_th*sizeof(unsigned char));

	#ifdef OUTPUT
	// Just printings
	print_params();
	print_selected_cpus();
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

		// Fixes thread to CPU
		cpu_set_t my_affinity;
		CPU_ZERO(&my_affinity);
		CPU_SET(my_cpu, &my_affinity);
		if(sched_setaffinity(0,sizeof(cpu_set_t),&my_affinity))
			set_affinity_error();

		#ifdef OUTPUT
		printf("I am thread (%d,%d) and I have got cpu %u\n", ompid, tid, my_cpu);
		#endif

		operation(ompid); // Kernel

		#ifdef OUTPUT
		printf("I am thread (%d,%d) in cpu %d and I just finished\n", ompid, tid, my_cpu);
		#endif
	}

	// Frees resources and end
	free(selected_cpus);
	numa_free(A, array_basic_size*sizeof(float)*num_th);
	numa_free(B, array_basic_size*sizeof(float)*num_th);
	numa_free(C, array_basic_size*sizeof(float)*num_th);
	return 0;
}
