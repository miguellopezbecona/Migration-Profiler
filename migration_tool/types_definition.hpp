#pragma once
#ifndef __TYPES__DEF__
#define __TYPES__DEF__

// pid_t is already defined in Linux as an int

using cpu_t  = size_t;
using tid_t  = pid_t;
using tim_t  = size_t;
using ins_t  = size_t;
using lat_t  = size_t;
using req_t  = size_t;
using node_t = pid_t;
using addr_t = size_t;
using dsrc_t = size_t;

#endif /* end of include guard: __TYPES__DEF__ */
