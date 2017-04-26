#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> // access, F_OK
#include <dirent.h> // Getting task folders from each process

#include <map>
#include <vector>
#include <algorithm> // sort
using namespace std;

#include "sample_data.h"

int get_median_from_list(vector<int> l);
bool is_migratable(pid_t my_uid, pid_t pid);
bool is_pid_alive(pid_t pid);
bool is_tid_alive(pid_t pid, pid_t tid);

/*** Time-related utils mainly used in my_profiler.c ***/
const int SYS_TIME_NUM_VALUES = 3;
const int sys_time_values[SYS_TIME_NUM_VALUES] = {1000, 2000, 4000}; // in ms
const double inv_1000 = 1 / 1000; // "* (1 / 1000)" should be faster than "/ 1000"

extern int current_time_value; // Indicates index to use in sys_time_values

void time_go_up();
void time_go_down();
int get_time_value();

void get_formatted_current_time(char *output);

/*** For getting children processes and writting them into a JSON file ***/
vector<pid_t> get_children_processes(pid_t pid);

void print_everything(vector<my_pebs_sample_t> samples, map<pid_t, vector<pid_t>> m);
