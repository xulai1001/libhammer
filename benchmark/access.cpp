#include "iostream"
#include "string"
#include "sstream"
#include "libhammer.h"

const string disk_name = "sda4";
const string vanilla_path = "memfile";
const string memway_path = "/tmp/libhammer/disk/memfile";
const int max_mb = 4200;

int avail_mb;

void waylaying_test(string path, uint64_t mb, bool warmup)
{
    uint64_t i;
    char *memfile=0;
    int fd, max_ns = 0;
    uint64_t memfile_size;
    struct stat st;
    volatile uint64_t tmp = 0;
    myclock clk, cl2;
//    DiskUsage du;

    // open eviction file
    ASSERT(-1 != (fd = open(path.c_str(), O_RDONLY | O_DIRECT)) );
    fstat(fd, &st);
    memfile_size = st.st_size;
    ASSERT(0 != (memfile = mmap(0, memfile_size, PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, 0)) );
    // cout << "- start eviction" << endl;
    if (warmup)
        cout << "- Warmup " << mb << " M..."; cout.flush();
    START_CLOCK(cl2, CLOCK_MONOTONIC);
    for (i=0; i<memfile_size; i+=PAGE_SIZE)
    {
        if (i > (mb << 20)) break;
        START_CLOCK(clk, CLOCK_MONOTONIC);
        tmp += *(volatile uint64_t *)(memfile + i);
        END_CLOCK(clk);
        if (!warmup)
        {
            if (clk.ns > max_ns) max_ns = clk.ns;
            if (i % (1<<20) == 0)
            {
                cout << max_ns << endl;
                max_ns = 0;
            }
        }
    }
    END_CLOCK(cl2);
    cout << "- " << ((cl2.ns / 1000000) / 1000.0) << " s" << endl;
    close(fd);
    munmap(memfile, memfile_size);
}

int main(void)
{
    const string pth = vanilla_path;

    avail_mb = get_available_mem() / 1024000;
    cout << "- Available memory: " << avail_mb << "M" << endl;

    waylaying_test(pth, avail_mb - 100, true);
    waylaying_test(pth, avail_mb - 100, true);
    waylaying_test(pth, avail_mb - 100, true);
    cout << "--------------------" << endl;
    waylaying_test(pth, max_mb, false);
    return 0;
}

