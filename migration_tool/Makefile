execname=my_profiler_tm

all:
	g++ -O2 -std=c++11 -o $(execname) [^ABC]*.c perfmon/*.c migration/*.c strategies/*.c strategies/genetic/*.c libpfm.so.4.8.0 -lnuma

search_warn:
	g++ -Wall -O2 -std=c++11 -o $(execname) [^ABC]*.c perfmon/*.c migration/*.c strategies/*.c strategies/genetic/*.c libpfm.so.4.8.0 -lnuma

