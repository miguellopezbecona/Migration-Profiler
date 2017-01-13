#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sched.h>
#include <sys/types.h>
#include <errno.h>

#include <sys/syscall.h>

#include <omp.h>

#include <numa.h>
#include <ctype.h> // isprint


/**
 ** VARIABEIS LANZADOR
**/
#define MAX_CPUS 4
#define MAX_TH 144

#define DEFAULT_MEM_CPU 0
#define DEFAULT_CPUS "000000000001"
#define DEFAULT_REP 1
#define DEFAULT_ARRAY_SIZE 100
#define DEFAULT_NUMBER_OPS 1
#define DEFAULT_STRIDE 1
int array_size;
int mem_cpu;
int num_th;
int rep;
int ops;
int stride;
unsigned char cpus[MAX_CPUS ];
unsigned char *mycpus;
//funcions
void set_mycpus();
void print_selected_cpus();
void print_params();
void usage(char **argv);
void set_affinity_error();
/**
 ** VARIABEIS PARA FUNCION
**/
float *A,*B,*C;
void reserva_memoria(){
	int i,j;
	cpu_set_t my_affinity;
	CPU_ZERO(&my_affinity);
	CPU_SET(mem_cpu,&my_affinity);
	if(sched_setaffinity(0,sizeof(cpu_set_t),&my_affinity)){
		set_affinity_error();
	}
	//sleep(1);
	A = numa_alloc_onnode(array_size*sizeof(float)*num_th, mem_cpu);
	B = numa_alloc_onnode(array_size*sizeof(float)*num_th, mem_cpu);
	C = numa_alloc_onnode(array_size*sizeof(float)*num_th, mem_cpu);
	for(j=0;j<num_th;j++){
		for(i=0;i<array_size;i++){
			A[j*array_size+i]= (rand()%10)*1.0;
			B[j*array_size+i]= (rand()%10)*1.0;
			C[j*array_size+i]= 0.0;
		}
/*
		printf("ABC: A[%d] from %lx to %lx\nB[%d] from %lx to %lx\nC[%d] from %lx to %lx\n",
			j,(long unsigned int)&A[j*array_size],(long unsigned int)&A[j*array_size+array_size],
			j,(long unsigned int)&B[j*array_size],(long unsigned int)&B[j*array_size+array_size],
			j,(long unsigned int)&C[j*array_size],(long unsigned int)&C[j*array_size+array_size]
		);
*/
	}
}
void funcion(){
	int i,j,k;
	pid_t	my_ompid = omp_get_thread_num();
	int my_index =my_ompid*array_size;
	for(j=0;j<rep;j++){
			for(i=0;i<array_size;i=i+stride){
				for(k=0;k<ops;k++){
					C[my_index+i]=A[my_index+i]*B[my_index+i];
				}
			}
		}
}




int main(int argc, char *argv[]){
	int i,j;
	char c;

	/**
	 ** Set defaults
	**/
	mem_cpu = DEFAULT_MEM_CPU;
	rep = DEFAULT_REP;
	ops = DEFAULT_NUMBER_OPS;
	array_size = DEFAULT_ARRAY_SIZE;
	stride = DEFAULT_STRIDE;
	char *defaultcpus = DEFAULT_CPUS;
	num_th=0;
	for(i=0;i<MAX_CPUS;i++){
		cpus[MAX_CPUS-1-i]=defaultcpus[i]-48;
		num_th+=cpus[MAX_CPUS-1-i];
	}
	/**
	 ** Input args
	**/
	if(argc<1){
		usage(argv);
		exit(-1);
	}else{ while ((c = getopt (argc, argv, "m:r:o:s:t:c:")) != -1)
      			switch (c)
      			{
       			case 'm':
		       {
		    	  	char *mc = optarg;
		     	  	mem_cpu = atoi(mc);
		       }
			    break;

		       case 'r':
		       {
		     		char *r = optarg;
		     	    	rep = atoi(r);
		       }
		     	    break;
			case 'o':
		       {
		     		char *o = optarg;
		     	    	ops = atoi(o);
		       }
		     	    break;

			case 't':
		       {
		     		char *t = optarg;
		     	    	stride = atoi(t);
		       }
		     	    break;

		       case 's':
		       {
		 		char *a = optarg;
		 	  	array_size = atol(a);
		       }
			    break;
			case 'c':
		       {
		 	 	char *cpuschar = optarg;
		 	  	if(strlen(cpuschar)!=MAX_CPUS){
					printf("Specify %d cpus\n",MAX_CPUS);
					exit(-1);
				}else{
					num_th=0;
					for(i=0;i<MAX_CPUS;i++){
						cpus[MAX_CPUS-1-i]=cpuschar[i]-48;
						num_th+=cpus[MAX_CPUS-1-i];
					}
				}
		       }
			    break;
		       case '?':
			    if (isprint (optopt))
				fprintf (stderr, "Unknown option `-%c'.\n", optopt);
			    else
				fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
			    	usage(argv);
		      }
	}
	set_mycpus();
	print_params();
	print_selected_cpus();

	/**
	 ** Seleccionar numero fios
	**/
	omp_set_num_threads(num_th);
	/**
	 ** Reservar memoria
	**/
	reserva_memoria();
	/**
	 ** LANZAR FIOS
	**/
	#pragma omp parallel private(i,j) firstprivate(array_size)
	{	
		cpu_set_t my_affinity;
		int my_tid=syscall(SYS_gettid);
		pid_t	my_pid = getpid();
		pid_t	my_ompid = omp_get_thread_num();
		unsigned char my_cpu=mycpus[my_ompid];

		printf("I am thread (%d,%d) and I have got cpu %u\n",my_ompid,my_tid,my_cpu);
		CPU_ZERO(&my_affinity);
		CPU_SET(my_cpu,&my_affinity);
		if(sched_setaffinity(0,sizeof(cpu_set_t),&my_affinity)){
			set_affinity_error();
		}
		funcion();
		printf("I am thread (%d,%d) in cpu %d and I just finished\n",my_ompid,my_tid,my_cpu);
	}
	
	return 0;
}
void set_mycpus(){
	mycpus = (unsigned char *) calloc(num_th,sizeof(unsigned char));
	unsigned char i,cpu_counter,remainder;
	cpu_counter=0;
	for(i=0;i<MAX_CPUS;i++){
		remainder=cpus[i];
		while(remainder>0){
			mycpus[cpu_counter] = i;
			cpu_counter++;
			remainder--;
		}
	}
}
			
void print_selected_cpus(){
	int i;
	for(i=0;i<MAX_CPUS;i++){
		if(cpus[i]!=0) printf("CPU %d selected: %u processes.\n",i,cpus[i]);
	}
}
void print_params(){
	printf("Initialising Data in  %d.\nRepetitions %d.\nArray Size %d.\nStride %d.\nNumber of Threads %d.\n",mem_cpu,rep,array_size,stride,num_th);
}
void usage(char **argv)
{
	printf("Usage: %s [-mmemory-cpu] [-rrepetitions] [-sarray_size] [-ccpus] [-tstride]\n\n",argv[0]);
}
void set_affinity_error()
{
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
