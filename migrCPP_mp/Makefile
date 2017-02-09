execname=my_profiler_tm

all:
	g++ -std=c++11 -o $(execname) [^ABC]*.c perfmon/*.c thread_migration/*.c page_migration/*.c libpfm.so.4.8.0 -lnuma

