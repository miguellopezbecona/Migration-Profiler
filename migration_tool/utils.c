#include "utils.h"

int get_median_from_list(vector<int> l){
	size_t size = l.size();
	sort(l.begin(), l.end()); // Getting median requires sorting

	if (size % 2 == 0)
		return (l[size / 2 - 1] + l[size / 2]) / 2;
	else 
		return l[size / 2];
}

// Useful to filter unwanted PIDs because unless you are root, you can't migrate processes you don't own
bool is_migratable(pid_t my_uid, pid_t pid){
	char folder[16];
	struct stat info;

	if(pid < 1) // Bad PID
		return false;
	if(my_uid == 0) // Root can do anything
		return true;

	sprintf(folder, "/proc/%d", pid);
	stat(folder, &info);
	return info.st_uid == (unsigned) my_uid;
}

bool is_pid_alive(pid_t pid){
	char dir[16] = "\0";
	sprintf(dir, "/proc/%d", pid);

	return access(dir, F_OK ) != -1;
}

bool is_tid_alive(pid_t pid, pid_t tid){
	char dir[32] = "\0";
	sprintf(dir, "/proc/%d/task/%d", pid, tid);

	return access(dir, F_OK ) != -1;
}

/*** Time-related utils ***/
int current_time_value = 0;
void time_go_up(){
	current_time_value++;
	if(current_time_value >= SYS_TIME_NUM_VALUES)
		current_time_value = SYS_TIME_NUM_VALUES - 1;
}

void time_go_down(){
	current_time_value--;
	if(current_time_value < 0)
		current_time_value = 0;
}

int get_time_value(){
	return sys_time_values[current_time_value];
}

void get_formatted_current_time(char *output){
	time_t rawtime = time(NULL);
	struct tm *timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	// Year-month-day. It's ugly but it's the correct way to sort correctly by filename later
	//sprintf(output, "%02d-%02d-%d_%02d-%02d-%02d", timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

	// Day-month-year
	sprintf(output, "%02d-%02d-%d_%02d-%02d-%02d",timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}

#ifdef JUST_PROFILE
/*** For getting children processes and writting them into a JSON file ***/
vector<pid_t> get_tids(pid_t pid){
	vector<pid_t> tids;
	pid_t tid;
	char folder[20] = "\0";
	struct dirent *buffer = NULL;
	DIR *dir = NULL;

	sprintf(folder, "/proc/%d/task/", pid);

	if( (dir=opendir(folder)) == NULL)
		return tids;

	while( (buffer = readdir(dir)) !=NULL){
		if(buffer->d_name[0] == '.') // No self-directory/parent
			continue;
		tid = atoi(buffer->d_name);
		tids.push_back(tid);
	}

	closedir(dir);

	return tids;
}

vector<pid_t> get_children_processes(pid_t pid){
	vector<pid_t> v;
	vector<pid_t> tids = get_tids(pid); // To know which task folders to search

	for(pid_t const & tid : tids){
		pid_t cpid;
		char filename[32] = "\0";
		FILE *file = NULL;

		sprintf(filename, "/proc/%d/task/%d/children", pid, tid);
		file = fopen(filename, "r");

		if(file == NULL)
			return v;

		while(fscanf(file, "%d", &cpid) == 1)
			v.push_back(cpid);
		fclose(file);
	}

	return v;
}

/*** For printing stuff ***/
void print_map_to_json(map<pid_t, vector<pid_t>> m, char const* base){
	// No data to dump
	if(m.empty())
		return;

	char filename[32];
	sprintf(filename, "%s.json", base);

	FILE* fp = fopen(filename, "w");

	if(fp == NULL){
		printf("Error opening file %s to log process structure.\n", filename);
		return;
	}

	fprintf(fp, "[\n");

	// To know when stop printing commas
	auto &m_last = *(--m.end());
	for(auto const & it : m){
		pid_t parent = it.first;
		vector<pid_t> children = it.second;

		fprintf(fp, "\t{ \"pid\" : %d, \"children\" : [\n\t\t", parent);

		auto &l_last = *(--children.end());
		for(pid_t const & c : children){
			fprintf(fp, " %d", c);

			if (&c != &l_last)
				fprintf(fp, ",");
		}
		fprintf(fp, "\n\t\t]\n\t}"); // End of children

		if (&it != &m_last)
			fprintf(fp, ",\n");

	}
	fprintf(fp, "]\n"); // End of file

	fclose(fp);
}

void print_samples(vector<my_pebs_sample_t> samples, const char* base){
	// No data to dump
	if(samples.empty())
		return;

	char filename[32];
	sprintf(filename, "%s.csv", base);

	FILE* fp = fopen(filename, "w");

	if(fp == NULL){
		printf("Error opening file %s to log samples.\n", filename);
		return;
	}

	my_pebs_sample_t::print_header(fp);
	for(my_pebs_sample_t const & s : samples)
		s.print(fp);

	fclose(fp);
}

void print_everything(vector<my_pebs_sample_t> samples, map<pid_t, vector<pid_t>> m){
	char base[32];
	get_formatted_current_time(base);

	print_samples(samples, base);

	#ifdef PRINT_JSON
	print_map_to_json(m, base);
	#endif
}
#endif
