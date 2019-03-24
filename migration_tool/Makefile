execbase=my_profiler
libpfmlink=libpfm.so.4.10.1
commonflags=-O2 -std=c++11 -g -fstack-protector-all
commonfiles=[^ABC]*.c perfmon/*.c migration/*.c

all:
	g++ $(commonflags) -o $(execbase)_tm $(commonfiles) strategies/*.c strategies/genetic/*.c rapl/*.c $(libpfmlink) -lnuma

# Strategies
str_ann:
	g++ $(commonflags) -o $(execbase)_tm $(commonfiles) -DUSE_ANNEA_ST strategies/annealing.c $(libpfmlink) -lnuma
str_rand:
	g++ $(commonflags) -o $(execbase)_tm $(commonfiles) -DUSE_RAND_ST strategies/random.c $(libpfmlink) -lnuma
str_gen:
	g++ $(commonflags) -o $(execbase)_tm $(commonfiles) -DUSE_GEN_ST strategies/genetic.c strategies/genetic/*.c $(libpfmlink) -lnuma
str_ener:
	g++ $(commonflags) -o $(execbase)_tm $(commonfiles) -DUSE_ENER_ST strategies/energy.c rapl/*.c $(libpfmlink) -lnuma
str_ft:
	g++ $(commonflags) -o $(execbase)_tm $(commonfiles) -DUSE_FIRST_ST strategies/first_touch.c $(libpfmlink) -lnuma

# Just profile modes
just_profile:
	g++ $(commonflags) -DJUST_PROFILE -o $(execbase)_jp [^ABC]*.c perfmon/*.c migration/inst_list.c migration/memory_list.c migration/system_struct.c migration/migration_facade.c $(libpfmlink) -lnuma

just_profile_energy:
	g++ $(commonflags) -DJUST_PROFILE -DJUST_PROFILE_ENERGY -o $(execbase)_jpe [^ABC]*.c perfmon/*.c migration/inst_list.c migration/memory_list.c migration/system_struct.c migration/migration_facade.c rapl/*.c $(libpfmlink) -lnuma

# all, but with -Wall
search_warn:
	g++ -Wall $(commonflags) -o $(execname) [^ABC]*.c perfmon/*.c migration/*.c strategies/*.c strategies/genetic/*.c rapl/*.c $(libpfmlink) -lnuma

# Fake data
fake_ann:
	g++ $(commonflags) -o $(execbase)_fake [^ABC]*.c -DFAKE_DATA -DUSE_ANNEA_ST perfmon/*.c migration/*.c strategies/annealing.c $(libpfmlink)
