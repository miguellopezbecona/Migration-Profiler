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
	FILE *fp;
	char command[40];
	char p_uid[6] = "\0";

	if(pid < 1) // Bad PID
		return false;
	if(my_uid == 0) // Root can do anything
		return true;

	// Command-dependent. Maybe it would be better to read /proc/pid/status file
	sprintf(command, "/bin/ps --no-headers -o uid -p %d", pid);
	fp = popen(command, "r");
	if (fp == NULL)
		return 0;

	fgets(p_uid, 1023, fp);
	pclose(fp);

	return atoi(p_uid) == my_uid;
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


