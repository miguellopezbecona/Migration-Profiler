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

