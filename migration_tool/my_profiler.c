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

/*
 * #define JUST_PROFILE, if you only want to use this tool for profiling
*/


#include <signal.h>
#include <getopt.h>
#include <setjmp.h>
#include <sys/wait.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "perfmon/perf_util.h"

#include "sample_data.h"
#include "migration/migration_facade.h" // begin_migration_process

#define NUM_GROUPS 2 // Changing this does not make the app work automatically
#define DEFAULT_MMAP_PAGES 16

#define MAX_PATH	1024
#ifndef STR
# define _STR(x) #x
# define STR(x) _STR(x)
#endif


typedef struct {
	int opt_no_show;
	int mmap_pages;
	int delay;
	int sbm;
	int th_mig;
	int pag_mig;
	char *eventsA;
	char *eventsB;
	char *cgroup;
//	int reduced_output;
	int periodA;
	int periodB;
	int minimum_latency;
} options_t;

unsigned int anti_inf_loop_counter = 0;
int uid; // For PID filtering

static jmp_buf jbuf;
static uint64_t collected_samples_total[NUM_GROUPS], lost_samples_total[NUM_GROUPS],processed_samples_total[NUM_GROUPS];

static int num_fds[NUM_GROUPS];
static options_t options;
static size_t sz, pgsz;
static size_t map_size;

static struct option the_options[]={
	{ "help", 0, 0,  1},
	{0,0,0,0}
};

static const char *gen_events[NUM_GROUPS] = {
	"MEM_TRANS_RETIRED:LATENCY_ABOVE_THRESHOLD:period=10000",
	"INSTRUCTIONS:period=10000000,OFFCORE_REQUESTS:ALL_DATA_RD" // Changed ALL_DATA_READ by ALL_DATA_RD
};

#ifndef JUST_PROFILE
/*
 * This is used for thread and page migration only
*/
int aux_counter=0;
time_t last_migration;


/*
 TIME COUNTING, move to file?
*/

#define SYS_TIME_NUM_VALUES 3
//in ms
#define SYS_TIME_VALUES const int sys_time_values[SYS_TIME_NUM_VALUES] = {1000, 2000, 4000};

SYS_TIME_VALUES
int current_time_value = 0;
static perf_event_desc_t **all_fds[NUM_GROUPS];


int time_go_up(){
	current_time_value++;
	if(current_time_value>=SYS_TIME_NUM_VALUES) current_time_value=SYS_TIME_NUM_VALUES-1;
	return 0;
}
int time_go_down(){
	current_time_value--;
	if(current_time_value<0) current_time_value=0;
	return 0;
}
int get_time_value(){
	return sys_time_values[current_time_value];
}
		

void perform_migration(){
	if(aux_counter==0){
		last_migration = time(NULL);
		aux_counter++;
		return;
	}
	time_t current_time = time(NULL);
	if((difftime(current_time,last_migration))>(get_time_value()/1000)){
		last_migration=current_time;
		//printf("\n***********\nAt %s\n",ctime(&last_migration));

		begin_migration_process(options.th_mig,options.pag_mig);
	}
}
#endif

static void process_smpl_buf(perf_event_desc_t *hw, int cpu, perf_event_desc_t **all_fds_p, int num_fds_p, int name) {
	my_pebs_sample_t my_sample;
	struct perf_event_header ehdr;
	perf_event_desc_t *fds = NULL;
	int ret;

	size_t to_skip; // ADDED

	//ADDED 
	fds = all_fds_p[cpu];
	my_sample.values = (uint64_t*)malloc(sizeof(uint64_t)*num_fds_p);	

	for(;;) {
		ret = perf_read_buffer(hw, &ehdr, sizeof(ehdr));
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
				//print_my_pebs_sample_for_3DyRM(&my_sample,options.output_file);
					
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
				to_skip = ehdr.size == 0 ? map_size : ehdr.size - sizeof(ehdr);
				//printf("Unknown sample type %d, Skipping %lu bytes.\n", ehdr.type, to_skip);

				perf_skip_buffer(hw, to_skip);

				anti_inf_loop_counter++;
				if(anti_inf_loop_counter>50){
					printf("error in read buffer, too many unknown samples, exiting\n");
					exit(-1);
				}
				break;
		}
		
	}
}

int setup_cpu(int cpu, int fd, int name) {
	perf_event_desc_t *fds = NULL;
	uint64_t *val;
	int ret, flags;

	// Allocate fds
	ret = perf_setup_list_events(gen_events[name], &fds, &num_fds[name]);
	if (ret || !num_fds[name])
		errx(1, "cannot setup event list");

	all_fds[name][cpu] = fds;
	
	//HERE DEFINE SPECIAL CONFIGURATION FOR EACH GROUP
	if(name==0){
		//Config for latency monitoring
		fds[0].hw.config1 = options.minimum_latency;
		fds[0].hw.sample_period	= options.periodA;	
		fds[0].hw.precise_ip = 2;
	} else
		fds[0].hw.sample_period	= options.periodB;

	if (!fds[0].hw.sample_period)
		errx(1, "need to set sampling period or freq on first event, use :period= or :freq=");

	fds[0].fd = -1;
	for(int i=0; i < num_fds[name]; i++) {
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
			if (num_fds[name] > 1)
				fds[i].hw.read_format |= PERF_FORMAT_GROUP|PERF_FORMAT_ID;
			//if (fds[i].hw.freq)
				//fds[i].hw.sample_type |= PERF_SAMPLE_PERIOD;
		}

		fds[i].hw.exclude_guest = 1;
		//lets exlcude kernel
		fds[i].hw.exclude_kernel = 1;

		fds[i].fd = perf_event_open(&fds[i].hw, -1, cpu, fds[0].fd, flags);
		if (fds[i].fd == -1) {
			if (fds[i].hw.precise_ip)
				err(1, "cannot attach event %s: precise mode may not be supported", fds[i].name);
			err(1, "cannot attach event %s", fds[i].name);
		}
	}

	/*
	 * kernel adds the header page to the size of the mmapped region
	 */
	fds[0].buf = mmap(NULL, map_size, PROT_READ|PROT_WRITE, MAP_SHARED, fds[0].fd, 0);
	if (fds[0].buf == MAP_FAILED)
		err(1, "cannot mmap buffer");

	/* does not include header page */
	fds[0].pgmsk = (options.mmap_pages*pgsz)-1;

	/*
	 * send samples for all events to first event's buffer
	 */
	for (int i = 1; i < num_fds[name]; i++) {
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
	if (num_fds[name] > 1) {
		sz = (3+2*num_fds[name])*sizeof(uint64_t);
		val = (uint64_t*)malloc(sz);
		if (!val)
			err(1, "cannot allocated memory");

		ret = read(fds[0].fd, val, sz);
		if (ret == -1)
			err(1, "cannot read id %zu", sizeof(val));

		for(int i=0; i < num_fds[name]; i++) {
			fds[i].id = val[2*i+1+3];
			printf("%lu  %s\n", fds[i].id, fds[i].name);
		}
		free(val);
	}
	return 0;
}

static void start_cpu(int c, perf_event_desc_t **all_fds_p) {
	perf_event_desc_t *fds = all_fds_p[c];

	if (fds[0].fd == -1)
		return;
	int ret = ioctl(fds[0].fd, PERF_EVENT_IOC_ENABLE, 0);
	if (ret)
		err(1, "cannot start counter");
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

static void handler(int n) {
	longjmp(jbuf, 1);
}

int mainloop(char **arg) {
	detect_system();
	uid = getuid();

	static uint64_t ovfl_count_total[NUM_GROUPS]; /* static to avoid setjmp issue */
	static uint64_t partial_read_total[NUM_GROUPS]; /* static to avoid setjmp issue */

	// This is the struct for polling the buffers of SYS_NUM_OF_CORES for 2 different groups of events
	struct pollfd pollfds[SYS_NUM_OF_CORES*NUM_GROUPS];
	int ret;
	int fd = -1;

	//set overflows to zero
	for(int i=0;i<NUM_GROUPS;i++)
		ovfl_count_total[i]=0;

	perf_event_desc_t *fds = NULL;

	//ADDED TO USE MORE GROUPS
	for(int i=0;i<NUM_GROUPS;i++){
		//ADDED TO USE ALL CPUS
		all_fds[i] = (perf_event_desc_t**)malloc(SYS_NUM_OF_CORES * sizeof(perf_event_desc_t *));
		if (!all_fds[i])
			err(1, "cannot allocate memory for all_fds[%d]",i);
	}

	if (pfm_initialize() != PFM_SUCCESS)
		errx(1, "libpfm initialization failed\n");

	pgsz = sysconf(_SC_PAGESIZE);
	map_size = (options.mmap_pages+1)*pgsz;

	if (options.cgroup) {
		fd = open_cgroup(options.cgroup);
		if (fd == -1)
			err(1, "cannot open cgroup file %s\n", options.cgroup);
	}

	//END ADDED
	for(int i=0;i<NUM_GROUPS;i++){
		for(int j=0;j<SYS_NUM_OF_CORES;j++)
			setup_cpu(j, fd,i);
	}

	signal(SIGALRM, handler);
	signal(SIGINT, handler);

	//This is for polling the buffers of SYS_NUM_OF_CORES cpus and the A group of events
	for(int i=0;i<SYS_NUM_OF_CORES;i++){
		fds = all_fds[0][i];
		pollfds[i].fd = fds[0].fd;
		pollfds[i].events = POLLIN;
	}
	
	//This is for polling the buffers of SYS_NUM_OF_CORES cpus and the B group of events
	for(int i=SYS_NUM_OF_CORES;i<SYS_NUM_OF_CORES*NUM_GROUPS;i++){
		fds = all_fds[1][i-SYS_NUM_OF_CORES];
		pollfds[i].fd = fds[0].fd;
		pollfds[i].events = POLLIN;
	}

	if (setjmp(jbuf) == 1){
		printf("TERMINATING\n");
		goto terminate_session;
	}

	for(int i=0;i<SYS_NUM_OF_CORES;i++){
		for(int j=0;j<NUM_GROUPS;j++)
			start_cpu(i,all_fds[j]);
	}

	// core loop HAS PROBLEMS
	for(;;) {

		ret = poll(pollfds, SYS_NUM_OF_CORES*2, options.sbm);
		if (ret < 0 && errno == EINTR)
			break;

		//timed out, read what we have got
		for(int i=0;i<NUM_GROUPS;i++){
			for(int j=0;j<SYS_NUM_OF_CORES;j++){
				process_smpl_buf(all_fds[i][j],j,all_fds[i],num_fds[i],i);
				partial_read_total[i]++;
			}
		}

		#ifndef JUST_PROFILE
		/*
		 * Here we should perform the migration, we have the data
		*/
		perform_migration();
		#endif
	}//end core loop
terminate_session:

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

	//This is information for the usual 2 groups (memory and instructions). Change if needed
/*
	printf("%lu(%lu) memory samples collected (processed) in total %lu poll events and %lu partial reads, %lu lost samples\n",collected_samples_total[0],processed_samples_total[0],ovfl_count_total[0],partial_read_total[0],lost_samples_total[0]);
	printf("%lu(%lu) instruction samples collected in total (processed) %lu poll events and %lu partial reads, %lu lost samples\n\n",collected_samples_total[1],processed_samples_total[1],ovfl_count_total[1],partial_read_total[1],lost_samples_total[1]);
*/
	printf("%d thread migrations made.\n", total_thread_migrations);
	printf("%d page migrations made.\n", total_page_migrations);

	return 0;
}

static void usage(void){
	printf("usage: my_profiler_tm [-h] [--help] [-m(do thread migration)] [-M(do page migration)] [-p period_memory] [-P period_indtructions] [-l minimum_latency] [-s seconds_between_migrations]\n");
}

int main(int argc, char **argv){
	int c;

	options.delay = -1;
//	options.reduced_output=0;
	options.periodA=1000;
	options.periodB=10000000;
	options.minimum_latency=200;
	options.sbm = 1;
	options.th_mig = 0;
	options.pag_mig = 0;

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
//				options.reduced_output = 1;
				break;
			case 'd':
				options.delay = atoi(optarg);
				break;
			case 'p':
				options.periodA = atoi(optarg);
				break;
			case 'P':
				options.periodB = atoi(optarg);
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

	if (!options.eventsA)
		options.eventsA = strdup(gen_events[0]);

	if (!options.eventsB)
		options.eventsB = strdup(gen_events[1]);

	if (!options.mmap_pages)
		options.mmap_pages = DEFAULT_MMAP_PAGES;

	if (options.delay == -1)
		options.delay = 10;

	if (options.mmap_pages > 1 && ((options.mmap_pages) & 0x1))
		errx(1, "number of pages must be power of 2\n");
	
	return mainloop(argv+optind); 
}
