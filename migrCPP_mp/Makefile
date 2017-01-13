CFLAGS =-I/home/remoto/oscar.garcia/mylib/libnuma/include -L/home/remoto/oscar.garcia/mylib/libnuma/lib -lnuma -lm

my_profiler_just_print:	my_profiler.c perf_util.c perf_util.h perf_event.h perf_event_open.h pfmlib_perf_event.h common_util.h common_util.c libpfm.a just_print/just_print.c
	gcc -o my_profiler my_profiler.c perf_util.c common_util.c just_print/just_print.c libpfm.a

my_profiler_profile:	my_profiler.c perf_util.c perf_util.h perf_event.h perf_event_open.h pfmlib_perf_event.h common_util.h common_util.c libpfm.a profile_util/profile_util.c profile_util/profile_util.h
	gcc -o my_profiler my_profiler.c perf_util.c common_util.c profile_util/profile_util.c libpfm.a

#my_profiler_3DyRM:	my_profiler.c perf_util.c perf_util.h perf_event.h perf_event_open.h pfmlib_perf_event.h common_util.h common_util.c libpfm.a
#	gcc -o my_profiler_3DyRM my_profiler.c perf_util.c common_util.c libpfm.a -DJUST_PROFILE

my_profiler_thread_migration:	my_profiler.c perf_util.c perf_util.h perf_event.h perf_event_open.h pfmlib_perf_event.h common_util.h common_util.c libpfm.a thread_migration/thread_migration.c thread_migration/thread_migration.h thread_migration/memory_list.c thread_migration/memory_list.h thread_migration/inst_list.c thread_migration/inst_list.h
	gcc -g -o my_profiler_tm my_profiler.c perf_util.c common_util.c thread_migration/*.c libpfm.a -lnuma -lm
	
my_profiler_thread_migration_opt:	my_profiler.c perf_util.c perf_util.h perf_event.h perf_event_open.h pfmlib_perf_event.h common_util.h common_util.c libpfm.a thread_migration/thread_migration.c thread_migration/thread_migration.h thread_migration/memory_list.c thread_migration/memory_list.h thread_migration/inst_list.c thread_migration/inst_list.h
	gcc -O2 -o my_profiler_tm my_profiler.c perf_util.c common_util.c thread_migration/*.c -I/usr/include -L/usr/lib libpfm.a $(CFLAGS)

clean:
	rm my_profiler
