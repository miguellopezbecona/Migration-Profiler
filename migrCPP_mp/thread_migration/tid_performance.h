#ifndef __TID_PERFORMANCE_H__
#define __TID_PERFORMANCE_H__

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>

//for pow
#include <math.h>

/* NEEDS to know the system for */
#include "system_struct.h"

#define DYRM_ALPHA 1
#define DYRM_BETA 1
#define DYRM_GAMMA 1

#define PERFORMANCE_INVALID_VALUE -1
//this is used when we do not have a measured latency
#define DEFAULT_LATENCY_FOR_CONSISTENCY 1000


typedef struct tid_performance{
	unsigned int step[SYS_NUM_OF_MEMORIES];
	unsigned int instructions[SYS_NUM_OF_MEMORIES];
	float mean_insts[SYS_NUM_OF_MEMORIES];
	float mean_instb[SYS_NUM_OF_MEMORIES];
	float mean_latency[SYS_NUM_OF_MEMORIES];
	unsigned int n_o_samples[SYS_NUM_OF_MEMORIES];
	float DyRM_performance[SYS_NUM_OF_MEMORIES];
	//may be normalised!
	float last_DyRM_performance;
	int core;//to know if it has changed
	
	tid_performance();

	//USE THESE FUNCTIONS TO ACCESS PERFORMANCE PER CORE
	void print();

	//This one does not care for history, always new
	//Right now rooflines and performance are calculated inside do_migration() in thread_migration.c, so we use up the information always
	int update(int core, unsigned int step, unsigned int instructions, float insts, float instb, float latency);

	int normalise_last_DyRM_performance(float mean_value);
	int reset();
} tid_performance_t;
#endif
