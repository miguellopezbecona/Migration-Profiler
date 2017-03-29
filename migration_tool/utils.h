#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // access, F_OK

#include <vector>
#include <algorithm> // sort
using namespace std;

int get_median_from_list(vector<int> l);

bool is_migratable(pid_t my_uid, pid_t pid);

bool is_pid_alive(pid_t pid);

bool is_tid_alive(pid_t pid, pid_t tid);

/*** Time-related utils used in my_profiler.c ***/
const int SYS_TIME_NUM_VALUES = 3;
const int sys_time_values[SYS_TIME_NUM_VALUES] = {1000, 2000, 4000}; // in ms
const double inv_1000 = 1 / 1000; // "* (1 / 1000)" should be faster than "/ 1000"

extern int current_time_value; // Indicates index to use in sys_time_values

void time_go_up();
void time_go_down();
int get_time_value();

