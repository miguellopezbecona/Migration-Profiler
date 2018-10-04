#pragma once

#include "../sample_data.h"
#include "memory_list.h"
#include "inst_list.h"
#include "pages_ops.h"
#include "migration_algorithm.h"

// How many samples do you want to log in the CSV files in JUST_PROFILE mode?
const int ELEMS_PER_PRINT = 1000000;

#define DO_MIGRATIONS

// Macros for some specific analysis
//#define MEAN_ACS_ANALY
//#define MEAN_LAT_ANALY

void add_data_to_list(my_pebs_sample_t sample);
int begin_migration_process();
void clean_migration_structures();

#ifdef JUST_PROFILE_ENERGY
void add_energy_data_to_last_sample();
#endif

void work_with_fake_data();

