/*
 * Based on syst_smpl.c - example of a system-wide sampling
 *
 * Copyright (c) 2010 Google, Inc
 * Contributed by Stephane Eranian <eranian@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <signal.h>
#include <getopt.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "perfmon/perf_util.h" // transfer_data_from_buffer_to_structure
#include "utils.h" // For time utils
#include "sample_data.h"
#include "migration/migration_facade.h" // begin_migration_process

// If you only want to use this tool for profiling, uncomment the following:
// #define JUST_PROFILE

// Using a value greater than 2 requires additional changes in, at least, "events" and "periods" array
#define NUM_GROUPS 1

// For the cgroups option, necessary?
#define MAX_PATH	1024
#ifndef STR
# define _STR(x) #x
# define STR(x) _STR(x)
#endif

typedef struct {
	int mmap_pages;
	int delay;
	int sbm;
	int th_mig;
	int pag_mig;
	char *cgroup;
	int periods[NUM_GROUPS];
	int minimum_latency;
} options_t;

unsigned int anti_inf_loop_counter = 0; // For unknown samples
int uid; // For PID filtering

static uint64_t collected_samples_total[NUM_GROUPS], lost_samples_total[NUM_GROUPS], processed_samples_total[NUM_GROUPS];
static uint64_t buffer_reads[NUM_GROUPS];

static int num_fds[NUM_GROUPS];
static options_t options;
static size_t sz, pgsz;
static size_t map_size;

static const char *events[2] = {
	"MEM_TRANS_RETIRED:LATENCY_ABOVE_THRESHOLD:period=10000"
	,"INSTRUCTIONS:period=10000000"
};

#ifndef JUST_PROFILE
static perf_event_desc_t **all_fds[NUM_GROUPS];

time_t last_migration;
void perform_migration(){

	time_t current_time = time(NULL);

	// We profile every 1, 2 or 4 seconds depending on current_time_value
	if((difftime(current_time,last_migration)) > (get_time_value() * inv_1000)){
		last_migration = current_time;
		//printf("\n***********\nAt %s\n",ctime(&last_migration));

		begin_migration_process(options.th_mig,options.pag_mig);
	}
}
#endif

static void process_smpl_buf(perf_event_desc_t *hw, int cpu, perf_event_desc_t **all_fds_p, int num_fds_p, int name) {
	my_pebs_sample_t my_sample;
	struct perf_event_header ehdr;
	perf_event_desc_t *fds = NULL;

	fds = all_fds_p[cpu];
	my_sample.values = (uint64_t*)malloc(sizeof(uint64_t)*num_fds_p);	

	for(;;) {
		int ret = perf_read_buffer(hw, &ehdr, sizeof(ehdr));
		if (ret)
			return; /* nothing to read */

		switch(ehdr.type) {
			case PERF_RECORD_SAMPLE:
				ret = transfer_data_from_buffer_to_structure(fds, num_fds_p, hw - fds, &ehdr, &my_sample, uid);
				processed_samples_total[name]++;

				if(ret < 0) // PID from process we can't migrate so it will be skipped
					break;

				#ifndef JUST_PROFILE
					add_data_to_list(my_sample);
				#endif
					
				if (ret)
					errx(1, "cannot parse sample");

				collected_samples_total[name]++;
				break;
			case PERF_RECORD_EXIT:
				//display_exit(hw, options.output_file);
				break;
			case PERF_RECORD_LOST:
				//lost_samples_total[name] += display_lost(hw, fds, num_fds_p,options.output_file);
				break;
			case PERF_RECORD_THROTTLE:
				//display_freq(1, hw, options.output_file);
				break;
			case PERF_RECORD_UNTHROTTLE:
				//display_freq(0, hw, options.output_file);
				break;
			default:
				// Skips different entire map page or partial
				size_t to_skip = ehdr.size == 0 ? map_size : ehdr.size - sizeof(ehdr);
				//printf("Unknown sample type %d, Skipping %lu bytes.\n", ehdr.type, to_skip);

				perf_skip_buffer(hw, to_skip);

				//anti_inf_loop_counter++;
				if(anti_inf_loop_counter>50){
					printf("error in read buffer, too many unknown samples, exiting\n");
					exit(-1);
				}
				break;
		}
		
	}
}

int setup_cpu(int cpu, int fd, int group) {
	perf_event_desc_t *fds = NULL;
	uint64_t *val;
	int ret, flags;

	// Allocate fds
	ret = perf_setup_list_events(events[group], &fds, &num_fds[group]);
	if (ret || !num_fds[group])
		errx(1, "cannot setup event list");

	all_fds[group][cpu] = fds;
	
	// Here we define special configuration for each group
	if(group == 0){ // Memory
		//Config for latency monitoring
		fds[0].hw.config1 = options.minimum_latency;
		fds[0].hw.precise_ip = 2;
	}
	fds[0].hw.sample_period	= options.periods[group];

	if (!fds[0].hw.sample_period)
		errx(1, "need to set sampling period or freq on first event, use :period= or :freq=");

	fds[0].fd = -1;
	for(int i=0; i < num_fds[group]; i++) {
		fds[i].hw.disabled = !i; // start immediately

		if (options.cgroup)
			flags = PERF_FLAG_PID_CGROUP;
		else
			flags = 0;

		if (fds[i].hw.sample_period) {
			// set notification threshold to be halfway through the buffer
			if (fds[i].hw.sample_period) {
				fds[i].hw.wakeup_watermark = (options.mmap_pages*pgsz) / 2;
				fds[i].hw.watermark = 1;
			}

			fds[i].hw.sample_type = PERF_SAMPLE_IP|PERF_SAMPLE_TID|PERF_SAMPLE_READ|PERF_SAMPLE_TIME|PERF_SAMPLE_PERIOD|PERF_SAMPLE_STREAM_ID|PERF_SAMPLE_ADDR|PERF_SAMPLE_CPU|PERF_SAMPLE_WEIGHT|PERF_SAMPLE_DATA_SRC;
			printf("%s period=%lu freq=%lu\n", fds[i].name, (long unsigned int) fds[i].hw.sample_period, fds[i].hw.freq);

			fds[i].hw.read_format = PERF_FORMAT_SCALE;
			if (num_fds[group] > 1)
				fds[i].hw.read_format |= PERF_FORMAT_GROUP|PERF_FORMAT_ID;
		}

		fds[i].hw.exclude_guest = 1;
		fds[i].hw.exclude_kernel = 1; // Let's exlcude kernel

		fds[i].fd = perf_event_open(&fds[i].hw, -1, cpu, fds[0].fd, flags); // Profile every PID for a given CPU
		if (fds[i].fd == -1) {
			if (fds[i].hw.precise_ip)
				err(1, "cannot attach event %s: precise mode may not be supported", fds[i].name);
			err(1, "cannot attach event %s", fds[i].name);
		}
	}

	// kernel adds the header page to the size of the mmapped region
	fds[0].buf = mmap(NULL, map_size, PROT_READ|PROT_WRITE, MAP_SHARED, fds[0].fd, 0);
	if (fds[0].buf == MAP_FAILED)
		err(1, "cannot mmap buffer");

	// does not include header page
	fds[0].pgmsk = (options.mmap_pages*pgsz)-1;

	// send samples for all events to first event's buffer
	for (int i = 1; i < num_fds[group]; i++) {
		if (!fds[i].hw.sample_period)
			continue;
		ret = ioctl(fds[i].fd, PERF_EVENT_IOC_SET_OUTPUT, fds[0].fd);
		if (ret)
			err(1, "cannot redirect sampling output");
	}

	/*
	 * we are using PERF_FORMAT_GROUP, therefore the structure
	 * of val is as follows:
	 *
	 *      { u64           nr;
	 *        { u64         time_enabled; } && PERF_FORMAT_ENABLED
	 *        { u64         time_running; } && PERF_FORMAT_RUNNING
	 *        { u64         value;
	 *          { u64       id;           } && PERF_FORMAT_ID
	 *        }             cntr[nr];
	 *      }
	 * We are skipping the first 3 values (nr, time_enabled, time_running)
	 * and then for each event we get a pair of values.
	 */
	if (num_fds[group] > 1) {
		sz = (3+2*num_fds[group])*sizeof(uint64_t);
		val = (uint64_t*)malloc(sz);
		if (!val)
			err(1, "cannot allocated memory");

		ret = read(fds[0].fd, val, sz);
		if (ret == -1)
			err(1, "cannot read id %zu", sizeof(val));

		for(int i=0; i < num_fds[group]; i++) {
			fds[i].id = val[2*i+1+3];
			printf("%lu  %s\n", fds[i].id, fds[i].name);
		}
		free(val);
	}
	return 0;
}

static const char* cgroupfs_find_mountpoint(void) {
	static char cgroup_mountpoint[MAX_PATH+1];
	FILE *fp;
	int found = 0;
	char type[64];

	fp = fopen("/proc/mounts", "r");
	if (!fp)
		return NULL;

	while (fscanf(fp, "%*s %"
				STR(MAX_PATH)
				"s %99s %*s %*d %*d\n",
				cgroup_mountpoint, type) == 2) {

		found = !strcmp(type, "cgroup");
		if (found)
			break;
	}
	fclose(fp);

	return found ? cgroup_mountpoint : NULL;
}

int open_cgroup(char *name) {
	char path[MAX_PATH+1];
	const char *mnt;
	int cfd;

	mnt = cgroupfs_find_mountpoint();
	if (!mnt)
		errx(1, "cannot find cgroup fs mount point");

	snprintf(path, MAX_PATH, "%s/%s", mnt, name);

	cfd = open(path, O_RDONLY);
	if (cfd == -1)
		warn("no access to cgroup %s\n", name);

	return cfd;
}

static void clean_end(int n) {
	perf_event_desc_t *fds = NULL;

	printf("TERMINATING\n");

	// Closes and frees resources	
	for(int i=0;i<NUM_GROUPS;i++){
		for(int j=0;j<SYS_NUM_OF_CORES;j++){
			fds = all_fds[i][j];
			for(int k=0; k < num_fds[i]; k++)
				close(fds[k].fd);

			munmap(fds[0].buf, map_size);
			perf_free_fds(fds, num_fds[i]);
		}
	}

	for(int i=0;i<NUM_GROUPS;i++)
		free(all_fds[i]);

	const char* types[2] = {"memory", "instruction"};
	for(int i=0;i<NUM_GROUPS;i++)
		printf("%lu (%lu) %s samples collected (processed) in total %lu poll events and %lu lost samples\n",collected_samples_total[i],processed_samples_total[i], types[i], buffer_reads[i], lost_samples_total[i]);

	printf("%d thread migrations made.\n", total_thread_migrations);
	printf("%d page migrations made.\n", total_page_migrations);

	exit(0);
}

int mainloop(char **arg) {
	// Obtains some important "constants" in execution time
	detect_system();
	uid = getuid();
	pgsz = sysconf(_SC_PAGESIZE);
	map_size = (options.mmap_pages+1)*pgsz;

	const unsigned short TOTAL_BUFFS = SYS_NUM_OF_CORES*NUM_GROUPS;
	last_migration = time(NULL);

	// This is the struct for polling the buffers of SYS_NUM_OF_CORES for different groups of events
	struct pollfd pollfds[TOTAL_BUFFS];
	int fd = -1;
	perf_event_desc_t *fds = NULL;

	// Sets buffer reads to zero
	memset(buffer_reads, 0, sizeof(buffer_reads));

	// Allocates memory for all_fds
	for(int i=0;i<NUM_GROUPS;i++){
		all_fds[i] = (perf_event_desc_t**)malloc(SYS_NUM_OF_CORES * sizeof(perf_event_desc_t *));
		if (!all_fds[i])
			err(1, "cannot allocate memory for all_fds[%d]",i);
	}

	// Initializes PFM
	if (pfm_initialize() != PFM_SUCCESS)
		errx(1, "libpfm initialization failed\n");

	// Can use cgroups (useful in this case?)
	if (options.cgroup) {
		fd = open_cgroup(options.cgroup);
		if (fd == -1)
			err(1, "cannot open cgroup file %s\n", options.cgroup);
	}

	// Sets up counter configuration
	for(int i=0;i<NUM_GROUPS;i++){
		for(int j=0;j<SYS_NUM_OF_CORES;j++)
			setup_cpu(j, fd, i);
	}

	// Sets up handler for some signals for a clean end
	signal(SIGALRM, clean_end);
	signal(SIGINT, clean_end);

	// This is for polling the buffers of SYS_NUM_OF_CORES cpus for the available groups
	for(int i=0;i<TOTAL_BUFFS;i++){
		int gr = i / SYS_NUM_OF_CORES;
		int cpu = i % SYS_NUM_OF_CORES;
		fds = all_fds[gr][cpu];
		pollfds[i].fd = fds[0].fd;
		pollfds[i].events = POLLIN;
	}

	// Starts counters
	for(int i=0;i<SYS_NUM_OF_CORES;i++){
		for(int j=0;j<NUM_GROUPS;j++){
			fds = all_fds[j][i];

			if (fds[0].fd == -1)
				continue;
			int ret = ioctl(fds[0].fd, PERF_EVENT_IOC_ENABLE, 0);
			if (ret)
				err(1, "cannot start counter");
		}
	}

	// Core loop where the polling to the buffers is done, has some issues
	for(;;) {

		int ret = poll(pollfds, TOTAL_BUFFS, options.sbm);
		if (ret < 0 && errno == EINTR)
			break;

		// Reads buffers
		for(int i=0;i<NUM_GROUPS;i++){
			for(int j=0;j<SYS_NUM_OF_CORES;j++){
				process_smpl_buf(all_fds[i][j], j, all_fds[i], num_fds[i], i);
				buffer_reads[i]++;
			}
		}

		#ifndef JUST_PROFILE
		// We have the data, so we can begin the migration process
		perform_migration();
		#endif
	}

	return 0;
}

static void usage(void){
	printf("usage: my_profiler_tm [-h] [--help] [-m(do thread migration)] [-M(do page migration)] [-p period_memory] [-P period_indtructions] [-l minimum_latency] [-s seconds_between_migrations]\n");
}

int main(int argc, char **argv){
	char c;

	// Defaults
	options.delay = -1;
	options.periods[0] = 1000;

	if(NUM_GROUPS > 1)
		options.periods[1] = 10000000;

	options.minimum_latency = 200;
	options.sbm = 1;
	options.th_mig = 0;
	options.pag_mig = 0;
	options.mmap_pages = 16;
	options.delay = 10;

	static struct option the_options[]= {
		{ "help", 0, 0,  1},
		{0,0,0,0}
	};

	while ((c=getopt_long(argc, argv,"+hrmMd:G:p:P:l:s:", the_options, 0)) != -1) {
		switch(c) {
			case 0: continue;
			case 'm':				
				options.th_mig = 1;
				//printf("Migrating threads\n");
				break;
			case 'M':
				options.pag_mig = 1;
				//printf("Migrating Pages\n");
				break;
			case 'r':
				//options.reduced_output = 1;
				break;
			case 'd':
				options.delay = atoi(optarg);
				break;
			case 'p':
				// This needs to be redefined in the future, probably with something like "period_g0;period_g1;"...
				options.periods[0] = atoi(optarg);
				break;
			case 'P':
				if(NUM_GROUPS > 1)
					options.periods[1] = atoi(optarg);
				break;
			case 'l':
				options.minimum_latency = atoi(optarg);
				break;
			case 'G':
				options.cgroup = optarg;
				break;
			case 's':
				options.sbm = atoi(optarg);
				break;
			case 'h':
				usage();
				exit(0);
			default:
				errx(1, "unknown option");
		}
	}

	if (options.mmap_pages > 1 && ((options.mmap_pages) & 0x1))
		errx(1, "number of pages must be power of 2\n");
	
	return mainloop(argv+optind); 
}
