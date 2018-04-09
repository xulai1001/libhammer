#ifndef _UTIL_H
#define _UTIL_H

#include "libhammer.h"
extern "C" {
    #include "sigsegv.h"
    #include "sys/mman.h"
    #include "sys/stat.h"
    #include "sys/wait.h"
}

using namespace std;

struct ImageFile {
    string name;
    size_t sz;
    char *image;
};

// get mem/cpu info
uint64_t get_mem_size();
uint64_t get_meminfo(const string &key);
uint64_t get_cached_mem();
uint64_t get_available_mem();
void set_cpu_affinity(int x);
string run_cmd(const char *cmd);
string get_cpu_model();
void continuation(void *a, void *b, void *c);
uint64_t get_binary_pa(const string &path);
void do_waylaying();
uint64_t v2p_once(void *v);
void interrupt(int sig);
ImageFile do_chasing(const string &path, uint64_t addr=0);
#endif
