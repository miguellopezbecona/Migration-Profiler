#pragma once
#ifndef __UTILS__
#define __UTILS__

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <unistd.h> // access, F_OK
#include <dirent.h> // Getting task folders from each process

#include <sys/stat.h> // stat

#include <iostream>
#include <fstream>

#include <map>
#include <vector>
#include <algorithm>

#include "sample_data.hpp"

// For JUST_PROFILE_MODE
//#define PRINT_JSON

inline int get_median_from_list (std::vector<int> l) {
	auto size = l.size();
	sort(l.begin(), l.end()); // Getting median requires sorting

	if (size % 2 == 0)
		return (l[size / 2 - 1] + l[size / 2]) / 2;
	else
		return l[size / 2];
}

inline bool is_migratable (const pid_t my_uid, const pid_t pid) {
	if (pid < 1) // Bad PID
		return false;
	if (my_uid == 0) // Root can do anything
		return true;

	char folder[16];
	struct stat info;

	sprintf(folder, "/proc/%d", pid);
	stat(folder, &info);
	return info.st_uid == (unsigned) my_uid;
}

inline bool is_pid_alive  (const pid_t pid) {
	char dir[16] = "\0";
	sprintf(dir, "/proc/%d", pid);

	return access(dir, F_OK ) != -1;
}

inline bool is_tid_alive  (const pid_t pid, const pid_t tid) {
	char dir[32] = "\0";
	sprintf(dir, "/proc/%d/task/%d", pid, tid);

	return access(dir, F_OK ) != -1;
}

/*** Time-related utils mainly used in my_profiler.c ***/
const int SYS_TIME_NUM_VALUES = 3;
const int sys_time_values[SYS_TIME_NUM_VALUES] = {1000, 2000, 4000}; // in ms
const double inv_1000 = 1.0 / 1000.0; // "* (1 / 1000)" should be faster than "/ 1000"

static int current_time_value = 0; // Indicates index to use in sys_time_values

inline void time_go_up () {
	current_time_value++;
	if (current_time_value >= SYS_TIME_NUM_VALUES)
		current_time_value = SYS_TIME_NUM_VALUES - 1;
}

inline void time_go_down () {
	current_time_value--;
	if (current_time_value < 0)
		current_time_value = 0;
}

inline int  get_time_value () {
	return sys_time_values[current_time_value];
}

inline void get_formatted_current_time (char * output) {
	time_t rawtime = time(NULL);
	struct tm * timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	// Year-month-day. It's ugly but it's the correct way to sort correctly by filename later
	//sprintf(output, "%02d-%02d-%d_%02d-%02d-%02d", timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

	// Day-month-year
	sprintf(output, "%02d-%02d-%d_%02d-%02d-%02d",
		timeinfo->tm_mday,
		timeinfo->tm_mon + 1,
		timeinfo->tm_year + 1900,
		timeinfo->tm_hour,
		timeinfo->tm_min,
		timeinfo->tm_sec);
}

#ifdef JUST_PROFILE
/*** For getting children processes and writting them into a JSON file ***/
std::vector<pid_t> get_tids (const pid_t pid) {
	std::vector<pid_t> tids;
	pid_t tid;
	char folder[20] = "\0";
	struct dirent * buffer = NULL;
	DIR * dir = NULL;

	sprintf(folder, "/proc/%d/task/", pid);

	if ( (dir = opendir(folder)) == NULL )
		return tids;

	while ( (buffer = readdir(dir)) != NULL ) {
		if (buffer->d_name[0] == '.') // No self-directory/parent
			continue;
		tid = atoi(buffer->d_name);
		tids.push_back(tid);
	}

	closedir(dir);

	return tids;
}

std::vector<pid_t> get_children_processes (const pid_t pid) {
	std::vector<pid_t> v;
	auto tids = get_tids(pid); // To know which task folders to search

	for (auto const & tid : tids) {
		char filename[32] = "\0";
		sprintf(filename, "/proc/%d/task/%d/children", pid, tid);

		std::ifstream file(filename);
		while (!file.eof()) {
			pid_t cpid;
			file >> cpid;
			v.push_back(cpid);
		}
		file.close();
	}

	return v;
}

/*** For printing stuff ***/
void print_map_to_json (const std::map<pid_t, std::vector<pid_t>> & m, char const * base) {
	// No data to dump
	if (m.empty())
		return;

	char filename[32];
	sprintf(filename, "%s.json", base);

	std::ofstream file(filename);

	if (!file.is_open()) {
		std::cerr << "Error opening file " << filename << " to log process structure." << '\n';
		return;
	}


	file << "[" << '\n';

	// To know when stop printing commas
	auto & m_last = *(--m.end());
	for (auto const & it : m) {
		auto parent = it.first;
		std::vector<pid_t> children = it.second;

		file << "\t{ \"pid\" : " << parent << ", \"children\" : [\n\t\t";

		auto & l_last = *(--children.end());
		for (auto const & c : children) {
			file << " " << c;

			if (&c != &l_last) {
				file << ",";
			}
		}
		file << "\n\t\t]\n\t}"; // End of children

		if (&it != &m_last) {
			file << "," << '\n';
		}

	}
	file << "]" << '\n'; // End of file

	file.close();
}

void print_samples (const std::vector<my_pebs_sample_t> & samples, const char * base){
	// No data to dump
	if (samples.empty())
		return;

	char filename[32];
	sprintf(filename, "%s.csv", base);

	std::ofstream file(filename);

	if (!file.is_open()) {
		std::cerr << "Error opening file " << filename << " to log samples." << '\n';
		return;
	}

	my_pebs_sample_t::print_header(file);
	for (auto const & s : samples) {
		file << s;
	}

	file.close();
}

void print_everything (const std::vector<my_pebs_sample_t> & samples, const std::map<pid_t, std::vector<pid_t>> & m){
	char base[32];
	get_formatted_current_time(base);

	print_samples(samples, base);

	#ifdef PRINT_JSON
	print_map_to_json(m, base);
	#endif
}
#endif

#ifdef JUST_PROFILE
size_t my_pebs_sample_t::max_nr = 0;
std::vector<char*> my_pebs_sample_t::inst_subevent_names;
#endif

#endif
