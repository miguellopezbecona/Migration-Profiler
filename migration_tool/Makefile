execname=my_profiler_tm

all:
	g++ -O2 -std=c++11 -o $(execname) [^ABC]*.c perfmon/*.c migration/*.c strategies/*.c strategies/genetic/*.c rapl/*.c libpfm.so.4.9.0 -lnuma

just_profile_energy:
	g++ -O2 -std=c++11 -DJUST_PROFILE_ENERGY -o my_profiler_jpe [^ABC]*.c perfmon/*.c migration/*.c strategies/*.c strategies/genetic/*.c rapl/*.c libpfm.so.4.9.0 -lnuma

search_warn:
	g++ -Wall -O2 -std=c++11 -o $(execname) [^ABC]*.c perfmon/*.c migration/*.c strategies/*.c strategies/genetic/*.c rapl/*.c libpfm.so.4.9.0 -lnuma


