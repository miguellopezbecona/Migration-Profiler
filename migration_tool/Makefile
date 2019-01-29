all: default just_profile just_profile_energy
execbase = my_profiler
libpfmlink = libpfm.so.4.9.0

CC = c++
INCLUDE = -I./
CPP_FLAGS = -std=c++17 -O3 -march=native #-g -O0 -Wall # Debug options
LIB_FLAGS = $(libpfmlink) -lnuma $(INCLUDE)

default:
	$(CC) $(CPP_FLAGS) -o $(execbase)_tm my_profiler.cpp $(LIB_FLAGS)

# Strategies
str_ann:
	$(CC) $(CPP_FLAGS) -o $(execbase)_tm [^ABC]*.cpp -DUSE_ANNEA_ST $(LIB_FLAGS)
str_rand:
	$(CC) $(CPP_FLAGS) -o $(execbase)_tm [^ABC]*.cpp -DUSE_RAND_ST $(LIB_FLAGS)
str_gen:
	$(CC) $(CPP_FLAGS) -o $(execbase)_tm [^ABC]*.cpp -DUSE_GEN_ST $(LIB_FLAGS)
str_ener:
	$(CC) $(CPP_FLAGS) -o $(execbase)_tm [^ABC]*.cpp -DUSE_ENER_ST $(LIB_FLAGS)
str_ft:
	$(CC) $(CPP_FLAGS) -o $(execbase)_tm [^ABC]*.cpp -DUSE_FIRST_ST $(LIB_FLAGS)

# Just profile modes
just_profile:
	$(CC) $(CPP_FLAGS) -DJUST_PROFILE -o $(execbase)_jp [^ABC]*.cpp $(LIB_FLAGS)

just_profile_energy:
	$(CC) $(CPP_FLAGS) -DJUST_PROFILE -DJUST_PROFILE_ENERGY -o $(execbase)_jpe [^ABC]*.cpp $(LIB_FLAGS)

# all, but with -Wall
search_warn:
	$(CC) -Wall $(CPP_FLAGS) -o $(execbase)_tm [^ABC]*.cpp $(LIB_FLAGS)
