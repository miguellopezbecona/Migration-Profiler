#pragma once
#ifndef __PERF_UTIL__
#define __PERF_UTIL__
/*
 * perf_util.c - helper functions for perf_events
 *
 * Copyright (c) 2009 Google, Inc
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
#include <sys/types.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#include "pfmlib_perf_event.h"
#include "utils.hpp"
#include "sample_data.hpp"
#include "migration/migration_facade.hpp" // Just for knowing if JUST_PROFILE macro was defined
// #include "perf_util.h"

typedef struct {
	struct perf_event_attr hw;
	uint64_t values[3];
	uint64_t prev_values[3];
	char *name;
	uint64_t id; /* event id kernel */
	void *buf;
	size_t pgmsk;
	int group_leader;
	int fd;
	int max_fds;
	int idx; /* opaque libpfm event identifier */
	char *fstr; /* fstr from library, must be freed */
} perf_event_desc_t;

/* handy shortcut */
#define PERF_FORMAT_SCALE (PERF_FORMAT_TOTAL_TIME_ENABLED|PERF_FORMAT_TOTAL_TIME_RUNNING)

static inline uint64_t perf_scale(uint64_t *values) {
	uint64_t res = 0;

	if (!values[2] && !values[1] && values[0])
		warnx("WARNING: time_running = 0 = time_enabled, raw count not zero\n");

	if (values[2] > values[1])
		warnx("WARNING: time_running > time_enabled\n");

	if (values[2])
		res = (uint64_t)((double)values[0] * values[1]/values[2]);
	return res;
}

static inline uint64_t perf_scale_delta(uint64_t *values, uint64_t *prev_values) {
	uint64_t res = 0;

	if (!values[2] && !values[1] && values[0])
		warnx("WARNING: time_running = 0 = time_enabled, raw count not zero\n");

	if (values[2] > values[1])
		warnx("WARNING: time_running > time_enabled\n");

	if (values[2] - prev_values[2])
		res = (uint64_t)((double)((values[0] - prev_values[0]) * (values[1] - prev_values[1])/ (values[2] - prev_values[2])));
	return res;
}


/*
 * TIME_RUNNING/TIME_ENABLED
 */
static inline double perf_scale_ratio(uint64_t *values) {
	if (!values[1])
		return 0.0;

	return values[2]*1.0/values[1];
}

static inline int perf_fd2event(perf_event_desc_t *fds, int num_events, int fd) {
	int i;

	for(i=0; i < num_events; i++)
		if (fds[i].fd == fd)
			return i;
	return -1;
}

/*
 * id = PERF_FORMAT_ID
 */
static inline int perf_id2event(perf_event_desc_t *fds, int num_events, uint64_t id) {
	int j;
	for(j=0; j < num_events; j++)
		if (fds[j].id == id)
			return j;
	return -1;
}

static inline int perf_is_group_leader(perf_event_desc_t *fds, int idx) {
	return fds[idx].group_leader == idx;
}

extern void perf_free_fds(perf_event_desc_t *fds, int num_fds) {
	int i;

	for (i = 0 ; i < num_fds; i++) {
		free(fds[i].name);
		free(fds[i].fstr);
	}
	free(fds);
}

/* the **fd parameter must point to a null pointer on the first call
 * max_fds and num_fds must both point to a zero value on the first call
 * The return value is success (0) vs. failure (non-zero)
 */
int perf_setup_argv_events(const char **argv, perf_event_desc_t **fds, int *num_fds) {
	perf_event_desc_t *fd;
	pfm_perf_encode_arg_t arg;
	int new_max, ret, num, max_fds;
	int group_leader;

	if (!(argv && fds && num_fds))
		return -1;

	fd = *fds;
	if (fd) {
		max_fds = fd[0].max_fds;
		if (max_fds < 2)
			return -1;
		num = *num_fds;
	} else {
		max_fds = num = 0; /* bootstrap */
	}
	group_leader = num;

	while(*argv) {
		if (num == max_fds) {
			if (max_fds == 0)
				new_max = 2;
			else
				new_max = max_fds << 1;

			if (new_max < max_fds) {
				warn("too many entries");
				goto error;
			}
			fd = (perf_event_desc_t*)realloc(fd, new_max * sizeof(*fd));
			if (!fd) {
				warn("cannot allocate memory");
				goto error;
			}
			/* reset newly allocated chunk */
			memset(fd + max_fds, 0, (new_max - max_fds) * sizeof(*fd));
			max_fds = new_max;

			/* update max size */
			fd[0].max_fds = max_fds;
		}
		/* ABI compatibility, set before calling libpfm */
		fd[num].hw.size = sizeof(fd[num].hw);

		memset(&arg, 0, sizeof(arg));
		arg.attr = &fd[num].hw;
		arg.fstr = &fd[num].fstr; /* fd[].fstr is NULL */

		ret = pfm_get_os_event_encoding(*argv, PFM_PLM0|PFM_PLM3, PFM_OS_PERF_EVENT_EXT, &arg);
		if (ret != PFM_SUCCESS) {
			warnx("event %s: %s", *argv, pfm_strerror(ret));
			goto error;
		}

		fd[num].name = strdup(*argv);
		fd[num].group_leader = group_leader;
		fd[num].idx = arg.idx;

		num++;
		argv++;
	}
	*num_fds = num;
	*fds = fd;
	return 0;
error:
	perf_free_fds(fd, num);
	return -1;
}

int perf_setup_list_events(const char *ev, perf_event_desc_t **fd, int *num_fds) {
	const char **argv;
	char *p, *q, *events;
	int i, ret, num = 0;

	if (!(ev && fd && num_fds))
		return -1;

	events = strdup(ev);
	if (!events)
		return -1;

	q = events;
	while((p = strchr(q, ','))) {
		num++;
		q = p + 1;
	}
	num++;
	num++; /* terminator */

	argv = (const char**)malloc(num * sizeof(char *));
	if (!argv) {
		free(events);
		return -1;
	}

	for(i=0, q = events; i < num-2; i++, q = p + 1) {
		p = strchr(q, ',');
		*p = '\0';
		argv[i] = q;
	}
	argv[i++] = q;
	argv[i] = NULL;
	ret = perf_setup_argv_events(argv, fd, num_fds);
	free(argv);
	free(events); /* strdup in perf_setup_argv_events() */
	return ret;
}

int perf_get_group_nevents(perf_event_desc_t *fds, int num, int idx) {
	int leader;
	int i;

	if (idx < 0 || idx >= num)
		return 0;

	leader = fds[idx].group_leader;

	for (i = leader + 1; i < num; i++) {
		if (fds[i].group_leader != leader) {
			/* This is a new group leader, so the previous
			 * event was the final event of the preceding
			 * group.
			 */
			return i - leader;
		}
	}
	return i - leader;
}

int perf_read_buffer(perf_event_desc_t *hw, void *buf, size_t sz) {
	struct perf_event_mmap_page *hdr = (perf_event_mmap_page*) hw->buf;
	size_t pgmsk = hw->pgmsk;
	char *data;
	unsigned long tail;
	size_t avail_sz, m, c;

	/*
	 * data points to beginning of buffer payload
	 */
	data = ((char *)hdr)+sysconf(_SC_PAGESIZE);

	/*
	 * position of tail within the buffer payload
	 */
	tail = hdr->data_tail & pgmsk;

	/*
	 * size of what is available
	 *
	 * data_head, data_tail never wrap around
	 */
	avail_sz = hdr->data_head - hdr->data_tail;
	if (sz > avail_sz)
		return -1;

	/*
	 * sz <= avail_sz, we can satisfy the request
	 */

	/*
	 * c = size till end of buffer
	 *
	 * buffer payload size is necessarily
	 * a power of two, so we can do:
	 */
	c = pgmsk + 1 -  tail;

	/*
	 * min with requested size
	 */
	m = c < sz ? c : sz;

	/* copy beginning */
	memcpy(buf, data+tail, m);

	/*
	 * copy wrapped around leftover
	 */
	if ((sz - m) > 0)
		memcpy( (char*)buf+m, data, sz - m);

	//printf("\nhead=%lx tail=%lx new_tail=%lx sz=%zu\n", hdr->data_head, hdr->data_tail, hdr->data_tail+sz, sz);
	hdr->data_tail += sz;

	return 0;
}

static inline int perf_read_buffer_32 (perf_event_desc_t *hw, void *buf) {
	return perf_read_buffer(hw, buf, sizeof(uint32_t));
}

static inline int perf_read_buffer_64(perf_event_desc_t *hw, void *buf) {
	return perf_read_buffer(hw, buf, sizeof(uint64_t));
}

void perf_skip_buffer(perf_event_desc_t *hw, size_t sz) {
	struct perf_event_mmap_page *hdr = (perf_event_mmap_page*) hw->buf;

	if ((hdr->data_tail + sz) > hdr->data_head)
		sz = hdr->data_head - hdr->data_tail;

	hdr->data_tail += sz;
}

static size_t __perf_handle_raw(perf_event_desc_t *hw) {
	size_t sz = 0;
	uint32_t raw_sz, i;
	char *buf;
	int ret;

	ret = perf_read_buffer_32(hw, &raw_sz);
	if (ret) {
		warnx("cannot read raw size");
		return -1;
	}

	sz += sizeof(raw_sz);

	printf("\n\tRAWSZ:%u\n", raw_sz);

	buf = (char*)malloc(raw_sz);
	if (!buf) {
		warn("cannot allocate raw buffer");
		return -1;
	}


	ret = perf_read_buffer(hw, buf, raw_sz);
	if (ret) {
		warnx("cannot read raw data");
		free(buf);
		return -1;
	}

	if (raw_sz)
		putchar('\t');

	for(i=0; i < raw_sz; i++) {
		printf("0x%02x ", buf[i] & 0xff );
		if (((i+1) % 16)  == 0)
			printf("\n\t");
	}
	if (raw_sz)
		putchar('\n');

	free(buf);

	return sz + raw_sz;
}

// IIP PID TID TIME SAMPLE_ADDR CPU WEIGHT grp.value (events 1--N) DSCR
int transfer_data_from_buffer_to_structure(perf_event_desc_t *fds, int num_fds, int idx, struct perf_event_header *ehdr, my_pebs_sample_t *sample, int uid) {
	perf_event_desc_t *hw;
	struct { uint32_t pid, tid; } pid;
	struct { uint64_t value, id; } grp;
	uint64_t time_enabled, time_running;
	size_t sz;
	uint64_t type, fmt;
	uint64_t val64;
	int ret;

	if (!fds || !ehdr  || num_fds < 0 || idx < 0 ||  idx >= num_fds)
		return -1;

	sz = ehdr->size - sizeof(*ehdr);

	hw = fds+idx;

	type = hw->hw.sample_type;
	fmt  = hw->hw.read_format;

	ret = perf_read_buffer_64(hw, &val64); // IIP
	sample->iip = val64;
	sz -= sizeof(val64);

	ret = perf_read_buffer(hw, &pid, sizeof(pid)); // TID, PID
	sample->tid = pid.tid;
	sample->pid = pid.pid;
	sz -= sizeof(pid);

	// If we are just profiling, we don't filter by PID
	#ifndef JUST_PROFILE
	if(!is_migratable(uid, pid.pid)){ // Important step: we skip the data if PID is about a process we can't migrate
		perf_skip_buffer(hw, sz);
		return -1;
	}
	#endif

	ret = perf_read_buffer_64(hw, &val64); // TIME
	sample->time = val64;
	sz -= sizeof(val64);

	ret = perf_read_buffer_64(hw, &val64); // ADDR
	sample->sample_addr = val64;
	sz -= sizeof(val64);

	ret = perf_read_buffer_64(hw, &val64); // STREAM_ID
	sz -= sizeof(val64);

	struct { uint32_t cpu, reserved; } cpu; // CPU
	ret = perf_read_buffer(hw, &cpu, sizeof(cpu));
	sample->cpu = cpu.cpu;
	sz -= sizeof(cpu);

	ret = perf_read_buffer_64(hw, &val64); // PERIOD
	sz -= sizeof(val64);

	uint64_t values[3];
	uint64_t nr;

	if (fmt & PERF_FORMAT_GROUP) { // Inst samples
		ret = perf_read_buffer_64(hw, &nr);
		sz -= sizeof(nr);
		time_enabled = time_running = 1;

		ret = perf_read_buffer_64(hw, &time_enabled); // TIME_ENABLED
		sz -= sizeof(time_enabled);

		ret = perf_read_buffer_64(hw, &time_running); // TIME_RUNNING
		sz -= sizeof(time_running);


		sample->time_enabled = time_enabled;
		sample->time_running = time_running;
		sample->nr = nr;

		values[1] = time_enabled;
		values[2] = time_running;
		while(nr--) {
			grp.id = -1;
			ret = perf_read_buffer_64(hw, &grp.value);
			sz -= sizeof(grp.value);

			ret = perf_read_buffer_64(hw, &grp.id); // Group ID
			sz -= sizeof(grp.id);

			values[0] = grp.value;
			grp.value = perf_scale(values);

			sample->values[nr] = grp.value;


		}
	} else { // Memory samples
		ret = perf_read_buffer_64(hw, &val64);
		sz -= sizeof(val64);

		ret = perf_read_buffer_64(hw, &time_enabled); // TIME_ENABLED
		sz -= sizeof(time_enabled);

		ret = perf_read_buffer_64(hw, &time_running); // TIME_RUNNING
		sz -= sizeof(time_running);

		sample->time_enabled = time_enabled;
		sample->time_running = time_running;
		sample->nr = 1;

		values[0] = val64;
		values[1] = time_enabled;
		values[2] = time_running;
		val64 = perf_scale(values);

		sample->values[0] = val64;
	}

	ret = perf_read_buffer_64(hw, &val64); // WEIGHT
	sample->weight = val64;
	sz -= sizeof(val64);

	ret = perf_read_buffer_64(hw, &val64); // DSRC
	sample->dsrc = val64;
	sz -= sizeof(val64);

	if (sz) {
		warnx("did not correctly parse sample leftover=%zu", sz);
		perf_skip_buffer(hw, sz);
	}

	return 0;
}

// Version with sanity-checking ifs
int transfer_data_from_buffer_to_structure_allifs(perf_event_desc_t *fds, int num_fds, int idx, struct perf_event_header *ehdr, my_pebs_sample_t *sample, int uid) {
	perf_event_desc_t *hw;
	struct { uint32_t pid, tid; } pid;
	struct { uint64_t value, id; } grp;
	uint64_t time_enabled, time_running;
	size_t sz;
	uint64_t type, fmt;
	uint64_t val64;
	int ret;

	if (!fds || !ehdr  || num_fds < 0 || idx < 0 ||  idx >= num_fds)
		return -1;

	sz = ehdr->size - sizeof(*ehdr);

	hw = fds+idx;

	type = hw->hw.sample_type;
	fmt  = hw->hw.read_format;

	/*
	 * the sample_type information is laid down
	 * based on the PERF_RECORD_SAMPLE format specified
	 * in the perf_event.h header file.
	 * That order is different from the enum perf_event_sample_format
	 */
	if (type & PERF_SAMPLE_IP) {
		ret = perf_read_buffer_64(hw, &val64);
		if (ret) {
			warnx("cannot read IP");
			return -1;
		}

		/*
		 * MISC_EXACT_IP indicates that kernel is returning
		 * th  IIP of an instruction which caused the event, i.e.,
		 * no skid
		 */
		//if (hw->hw.precise_ip && (ehdr->misc & PERF_RECORD_MISC_EXACT_IP)) // (exact)

		sample->iip = val64;
		sz -= sizeof(val64);
	} else
		sample->iip = 0;


	if (type & PERF_SAMPLE_TID) {
		ret = perf_read_buffer(hw, &pid, sizeof(pid));
		if (ret) {
			warnx("cannot read PID");
			return -1;
		}

		sample->tid = pid.tid;
		sample->pid = pid.pid;
		sz -= sizeof(pid);

		// If we are just profiling, we don't filter by PID
		#ifndef JUST_PROFILE
		if(!is_migratable(uid, pid.pid)){ // Important step: we skip the data if PID is about a process we can't migrate
			perf_skip_buffer(hw, sz);
			return -1;
		}
		#endif
	}

	if (type & PERF_SAMPLE_TIME) {
		ret = perf_read_buffer_64(hw, &val64);
		if (ret) {
			warnx("cannot read time");
			return -1;
		}

		sample->time = val64;
		sz -= sizeof(val64);
	}

	if (type & PERF_SAMPLE_ADDR) {
		ret = perf_read_buffer_64(hw, &val64);
		if (ret) {
			warnx("cannot read addr");
			return -1;
		}

		sample->sample_addr = val64;
		sz -= sizeof(val64);
	}

	if (type & PERF_SAMPLE_ID) {
		ret = perf_read_buffer_64(hw, &val64);
		if (ret) {
			warnx("cannot read id");
			return -1;
		}

		sz -= sizeof(val64);
	}

	if (type & PERF_SAMPLE_STREAM_ID) {
		ret = perf_read_buffer_64(hw, &val64);
		if (ret) {
			warnx("cannot read stream_id");
			return -1;
		}

		sz -= sizeof(val64);
	}

	if (type & PERF_SAMPLE_CPU) {
		struct { uint32_t cpu, reserved; } cpu;
		ret = perf_read_buffer(hw, &cpu, sizeof(cpu));
		if (ret) {
			warnx("cannot read cpu");
			return -1;
		}

		sample->cpu = cpu.cpu;
		sz -= sizeof(cpu);
	}


	if (type & PERF_SAMPLE_PERIOD) {
		ret = perf_read_buffer_64(hw, &val64);
		if (ret) {
			warnx("cannot read period");
			return -1;
		}

		sz -= sizeof(val64);
	}

	/* struct read_format {
	 * 	{ u64		value;
	 * 	  { u64		time_enabled; } && PERF_FORMAT_ENABLED
	 * 	  { u64		time_running; } && PERF_FORMAT_RUNNING
	 * 	  { u64		id;           } && PERF_FORMAT_ID
	 * 	} && !PERF_FORMAT_GROUP
	 *
	 * 	{ u64		nr;
	 * 	  { u64		time_enabled; } && PERF_FORMAT_ENABLED
	 * 	  { u64		time_running; } && PERF_FORMAT_RUNNING
	 * 	  { u64		value;
	 * 	    { u64	id;           } && PERF_FORMAT_ID
	 * 	  }		cntr[nr];
	 * 	} && PERF_FORMAT_GROUP
	 * };
	 */
	if (type & PERF_SAMPLE_READ) {
		uint64_t values[3];
		uint64_t nr;

		if (fmt & PERF_FORMAT_GROUP) {
			ret = perf_read_buffer_64(hw, &nr);
			if (ret) {
				warnx("cannot read nr");
				return -1;
			}

			sz -= sizeof(nr);

			time_enabled = time_running = 1;

			if (fmt & PERF_FORMAT_TOTAL_TIME_ENABLED) {
				ret = perf_read_buffer_64(hw, &time_enabled);
				if (ret) {
					warnx("cannot read timing info");
					return -1;
				}
				sz -= sizeof(time_enabled);
			}

			if (fmt & PERF_FORMAT_TOTAL_TIME_RUNNING) {
				ret = perf_read_buffer_64(hw, &time_running);
				if (ret) {
					warnx("cannot read timing info");
					return -1;
				}
				sz -= sizeof(time_running);
			}

			sample->time_enabled = time_enabled;
			sample->time_running = time_running;
			sample->nr = nr;

			values[1] = time_enabled;
			values[2] = time_running;
			while(nr--) {
				grp.id = -1;
				ret = perf_read_buffer_64(hw, &grp.value);
				if (ret) {
					warnx("cannot read group value");
					return -1;
				}
				sz -= sizeof(grp.value);

				if (fmt & PERF_FORMAT_ID) {
					ret = perf_read_buffer_64(hw, &grp.id);
					if (ret) {
						warnx("cannot read leader id");
						return -1;
					}
					sz -= sizeof(grp.id);
				}

				values[0] = grp.value;
				grp.value = perf_scale(values);

				sample->values[nr] = grp.value;


			}
		} else {
			/*
			 * this program does not use FORMAT_GROUP when there is only one event
			 */
			ret = perf_read_buffer_64(hw, &val64);
			if (ret) {
				warnx("cannot read value");
				return -1;
			}
			sz -= sizeof(val64);

			if (fmt & PERF_FORMAT_TOTAL_TIME_ENABLED) {
				ret = perf_read_buffer_64(hw, &time_enabled);
				if (ret) {
					warnx("cannot read timing info");
					return -1;
				}
				sz -= sizeof(time_enabled);
			}

			if (fmt & PERF_FORMAT_TOTAL_TIME_RUNNING) {
				ret = perf_read_buffer_64(hw, &time_running);
				if (ret) {
					warnx("cannot read timing info");
					return -1;
				}
				sz -= sizeof(time_running);
			}
			if (fmt & PERF_FORMAT_ID) {
				ret = perf_read_buffer_64(hw, &val64);
				if (ret) {
					warnx("cannot read leader id");
					return -1;
				}
				sz -= sizeof(val64);
			}

			sample->time_enabled = time_enabled;
			sample->time_running = time_running;
			sample->nr = 1;

			values[0] = val64;
			values[1] = time_enabled;
			values[2] = time_running;
			val64 = perf_scale(values);

			sample->values[0] = val64;

		}
	}

	if (type & PERF_SAMPLE_CALLCHAIN) {
		uint64_t nr, ip;

		ret = perf_read_buffer_64(hw, &nr);
		if (ret) {
			warnx("cannot read callchain nr");
			return -1;
		}
		sz -= sizeof(nr);

		while(nr--) {
			ret = perf_read_buffer_64(hw, &ip);
			if (ret) {
				warnx("cannot read ip");
				return -1;
			}

			sz -= sizeof(ip);
		}
	}

	if (type & PERF_SAMPLE_RAW) {
		ret = __perf_handle_raw(hw);
		if (ret == -1)
			return -1;
		sz -= ret;
	}

	if (type & PERF_SAMPLE_WEIGHT) {
		ret = perf_read_buffer_64(hw, &val64);
		if (ret) {
			warnx("cannot read weight");
			return -1;
		}

		sample->weight = val64;
		sz -= sizeof(val64);
	}

	if (type & PERF_SAMPLE_DATA_SRC) {
		ret = perf_read_buffer_64(hw, &val64);
		if (ret) {
			warnx("cannot read dsrc");
			return -1;
		}

		sample->dsrc = val64;

		sz -= sizeof(val64);
	}

	/*
	 * if we have some data left, it is because there is more
	 * than what we know about. In fact, it is more complicated
	 * because we may have the right size but wrong layout. But
	 * that's the best we can do.
	 */
	if (sz) {
		warnx("did not correctly parse sample leftover=%zu", sz);
		perf_skip_buffer(hw, sz);
	}

	return 0;
}

uint64_t display_lost(perf_event_desc_t *hw, perf_event_desc_t *fds, int num_fds, FILE *fp) {
	struct { uint64_t id, lost; } lost;
	const char *str;
	int e, ret;

	ret = perf_read_buffer(hw, &lost, sizeof(lost));
	if (ret) {
		warnx("cannot read lost info");
		return 0;
	}

	e = perf_id2event(fds, num_fds, lost.id);
	if (e == -1)
		str = "unknown lost event";
	else
		str = fds[e].name;

	fprintf(fp, "<<<LOST %lu SAMPLES FOR EVENT %s>>>\n", lost.lost, str);

	return lost.lost;
}

void display_exit(perf_event_desc_t *hw, FILE *fp) {
	struct { pid_t pid, ppid, tid, ptid; } grp;
	int ret;

	ret = perf_read_buffer(hw, &grp, sizeof(grp));
	if (ret) {
		warnx("cannot read exit info");
		return;
	}

	fprintf(fp,"[%d] exited\n", grp.pid);
}

void display_freq(int mode, perf_event_desc_t *hw, FILE *fp) {
	struct { uint64_t time, id, stream_id; } thr;
	int ret;

	ret = perf_read_buffer(hw, &thr, sizeof(thr));
	if (ret) {
		warnx("cannot read throttling info");
		return;
	}

	fprintf(fp, "%s value=%lu event ID=%lu\n",
		mode ? "Throttled" : "Unthrottled",
		thr.id,
		thr.stream_id);
}

#endif
