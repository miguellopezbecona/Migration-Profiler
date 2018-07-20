execbase=my_profiler
libpfmlink=libpfm.so.4.9.0

all:
	g++ -O2 -std=c++11 -o $(execbase)_tm [^ABC]*.c perfmon/*.c migration/*.c strategies/*.c strategies/genetic/*.c rapl/*.c $(libpfmlink) -lnuma

just_profile:
	g++ -O2 -std=c++11 -DJUST_PROFILE -o $(execbase)_jp [^ABC]*.c perfmon/*.c migration/inst_list.c migration/memory_list.c migration/system_struct.c migration/migration_facade.c $(libpfmlink) -lnuma

just_profile_energy:
	g++ -O2 -std=c++11 -DJUST_PROFILE -DJUST_PROFILE_ENERGY -o $(execbase)_jpe [^ABC]*.c perfmon/*.c migration/inst_list.c migration/memory_list.c migration/system_struct.c migration/migration_facade.c rapl/*.c $(libpfmlink) -lnuma

search_warn:
	g++ -Wall -O2 -std=c++11 -o $(execname) [^ABC]*.c perfmon/*.c migration/*.c strategies/*.c strategies/genetic/*.c rapl/*.c $(libpfmlink) -lnuma


