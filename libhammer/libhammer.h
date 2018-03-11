#ifndef _LIB_HAMMER
#define _LIB_HAMMER

#include "iostream"
#include "sstream"
#include "cstdio"
#include "cstdlib"
#include "cstring"
#include "vector"
#include "map"
#include "string"
#include "memory"
#include "algorithm"

extern "C" {
    #include "unistd.h"
    #include "errno.h"
    #include "stdint.h"
    #include "fcntl.h"
    #include "sys/mman.h"
    #include "time.h"
    #include "linux/types.h"
    #include "sys/stat.h"
    #include "sys/sysinfo.h"
    #include "sys/ipc.h"
    #include "sys/shm.h"
    #include "sys/types.h"
    #include "sched.h"
    #include "sigsegv.h"

    #include "timing.h"
    #include "asm.h"
    #include "memory.h"
}

#include "page.h"
#include "addrmap.h"
#include "utils.h"

using namespace std;


#endif
