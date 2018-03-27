#include "libhammer.h"

using namespace std;

uint64_t target_pa, mem_size, target_offset = 0;
char *memfile, *target;
int fd;

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
    cout << "- mem size: " << dec << (mem_size>>20) << " M" << endl;

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

void test_pcache_size()
{
    int64_t i, t;

    for (i=mem_size - 1; i > 0; i-=(100 << 20))
    {
        t = access_time(i);
        cout << dec << (i>>20)<< "M - " << t << endl;
    }
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
    test_pcache_size();

    cout << "---" << endl;
    cout << "path: " << fname << ", pa=" << hex << get_binary_pa(fname) << " target paddr=" << target_pa << endl;

    cleanup();

    return 0;
}
