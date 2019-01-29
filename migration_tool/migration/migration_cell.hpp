#pragma once
#ifndef __MIGRATTION_CELL__
#define __MIGRATTION_CELL__

#include <iostream>

#include <cstdio>
#include <cstdlib>
#include <unistd.h>

// For numa_move_pages
#include <numa.h>
#include <numaif.h>

#include "system_struct.hpp"

//#define PG_MIGR_OUTPUT
#define TH_MIGR_OUTPUT

// We need to know what to migrate and where
class migration_cell_t {
public:
	static int total_thread_migrations;
	static int total_page_migrations;

	long int elem; // Core or page address
	short dest; // Mem node for pages or core for TIDs
	short prev_dest; // Useful for undoing migrations
	pid_t pid;
	bool thread_cell;

	migration_cell_t () {};

	migration_cell_t (long int elem, short dest) :
		elem(elem),
		dest(dest),
		prev_dest(-1),
		pid(-1),
		thread_cell(true) // If we don't use a PID, it HAS to be a thread cell
	{};

	migration_cell_t (long int elem, short dest, pid_t pid, bool thread_cell) :
		elem(elem),
		dest(dest),
		prev_dest(-1),
		pid(pid),
		thread_cell(thread_cell)
	{};

	migration_cell_t (long int elem, short dest, short prev_dest, pid_t pid, bool thread_cell) :
		elem(elem),
		dest(dest),
		prev_dest(prev_dest),
		pid(pid),
		thread_cell(thread_cell)
	{};

	inline bool is_thread_cell () const {
		return thread_cell;
	}

	int perform_page_migration () const {
		void **page = (void **)calloc(1,sizeof(long int *));
		page[0] = (void*) elem;
		int dest_int = dest, status;

		// Key system call: numa_move_pages
		int ret = numa_move_pages(pid, 1, page, &dest_int, &status, MPOL_MF_MOVE);
		if (ret < 0){
			std::cout << "Move pages did not work: " << strerror(errno) << '\n';
			return errno;
		}

		total_page_migrations++;

		#ifdef PG_MIGR_OUTPUT
		std::cout << "Migrated page number " << (long unsigned int) elem " to node " << dest << ", status=" << status;
		if (status < 0) {
			std::cout << ", err: " << strerror(-status);
		}
		std::cout << '\n'
		#endif
	}

	int perform_thread_migration () const {
		int ret = system_struct_t::set_tid_cpu((pid_t) elem, dest, true);

		#ifdef TH_MIGR_OUTPUT
		std::cout << "Migrated thread " << pid_t(elem) << " to CPU " << dest + 1 << '\n'; // +1 only for demo!
		#endif

		total_thread_migrations++;

		return ret;
	}

	int perform_migration () const { // Can make private the two methods above
		if(is_thread_cell())
			return perform_thread_migration();
		else
			return perform_page_migration();
	}

	inline void interchange_dest () {
		std::swap(dest, prev_dest);
	}

	void print () const {
		std::cout << *this;
	}


	friend std::ostream & operator << (std::ostream & os, const migration_cell_t & mc) {
		const char * const types[] = {"Page", "Thread"};
		const char * const elems[] = {"memory page", "TID"};
		const char * const to_migrates[] = {"memory node", "CPU"};

		bool is_thread = mc.is_thread_cell();

		os << types[is_thread] << " migration cell. " << elems[is_thread] << " " << mc.elem << "to be migrated to " << to_migrates[is_thread] << " " << mc.dest << '.';

		if(mc.pid > 0)
			os << " PID: " << mc.pid << ".";
		if(mc.prev_dest > -1)
			os << " It was in " << to_migrates[is_thread] << " " << mc.prev_dest << ".";

		os << '\n';

		return os;
	}

};

int migration_cell_t::total_thread_migrations = 0;
int migration_cell_t::total_page_migrations = 0;

#endif
