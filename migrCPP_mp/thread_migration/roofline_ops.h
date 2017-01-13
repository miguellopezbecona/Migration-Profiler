#ifndef __ROOFLINE_OPS_H__
#define __ROOFLINE_OPS_H__


#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>

/* USES all lists*/
#include "memory_list.h"
#include "inst_list.h"
#include "pid_list.h"
#include "tid_list.h"
#include "temp_tid_list.h"
#include "system_struct.h"

#include "tid_performance.h"

//the inst_data_list has to have the increments created
int rooflines(unsigned int step, memory_data_list_t* memory_list, inst_data_list_t inst_list, pid_list_t* pid_m);

#endif
