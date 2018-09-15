execbase=my_profiler
libpfmlink=libpfm.so.4.9.0

all:
	g++ -O2 -std=c++11 -o $(execbase)_tm [^ABC]*.c perfmon/*.c migration/*.c strategies/*.c strategies/genetic/*.c rapl/*.c $(libpfmlink) -lnuma

# Strategies
str_ann:
	g++ -O2 -std=c++11 -o $(execbase)_tm [^ABC]*.c -DUSE_ANNEA_ST perfmon/*.c migration/*.c strategies/annealing.c $(libpfmlink) -lnuma
str_rand:
	g++ -O2 -std=c++11 -o $(execbase)_tm [^ABC]*.c -DUSE_RAND_ST perfmon/*.c migration/*.c strategies/random.c $(libpfmlink) -lnuma
str_gen:
	g++ -O2 -std=c++11 -o $(execbase)_tm [^ABC]*.c -DUSE_GEN_ST perfmon/*.c migration/*.c strategies/genetic.c strategies/genetic/*.c $(libpfmlink) -lnuma
str_ener:
	g++ -O2 -std=c++11 -o $(execbase)_tm [^ABC]*.c -DUSE_ENER_ST perfmon/*.c migration/*.c strategies/energy.c rapl/*.c $(libpfmlink) -lnuma
str_ft:
	g++ -O2 -std=c++11 -o $(execbase)_tm [^ABC]*.c -DUSE_FIRST_ST perfmon/*.c migration/*.c strategies/first_touch.c $(libpfmlink) -lnuma

# Just profile modes
just_profile:
	g++ -O2 -std=c++11 -DJUST_PROFILE -o $(execbase)_jp [^ABC]*.c perfmon/*.c migration/inst_list.c migration/memory_list.c migration/system_struct.c migration/migration_facade.c $(libpfmlink) -lnuma

just_profile_energy:
	g++ -O2 -std=c++11 -DJUST_PROFILE -DJUST_PROFILE_ENERGY -o $(execbase)_jpe [^ABC]*.c perfmon/*.c migration/inst_list.c migration/memory_list.c migration/system_struct.c migration/migration_facade.c rapl/*.c $(libpfmlink) -lnuma

# all, but with -Wall
search_warn:
	g++ -Wall -O2 -std=c++11 -o $(execname) [^ABC]*.c perfmon/*.c migration/*.c strategies/*.c strategies/genetic/*.c rapl/*.c $(libpfmlink) -lnuma


