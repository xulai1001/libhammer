#ifndef _UTIL_H
#define _UTIL_H

#include "libhammer.h"
extern "C" {
    #include "sigsegv.h"
    #include "sys/mman.h"
    #include "sys/stat.h"
}

using namespace std;

struct ImageFile {
    string name;
    size_t sz;
    char *image;
};

// get mem/cpu info
uint64_t get_mem_size();
void set_cpu_affinity(int x);
string run_cmd(const char *cmd);
string get_cpu_model();
void continuation(void *a, void *b, void *c);
uint64_t get_binary_pa(const string &path);
void do_waylaying();
ImageFile do_chasing(const string &path, uint64_t addr=0);
#endif
