#include "iostream"
#include "string"
#include "sstream"
#include "libhammer.h"

void vanilla_waylaying()
{
    uint64_t i, cached_ns, uncached_ns;
    char *memfile=0;
    int fd;
    uint64_t memfile_size;
    struct stat st;
    volatile uint64_t tmp = 0;
    myclock clk, cl2;
    DiskUsage du;

    // open eviction file
    ASSERT(-1 != (fd = open("memfile", O_RDONLY | O_DIRECT)) );
    fstat(fd, &st);
    memfile_size = st.st_size;
    ASSERT(0 != (memfile = mmap(0, memfile_size, PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, 0)) );

    // cout << "- start eviction" << endl;
    du.init("sda4");
    START_CLOCK(cl2, CLOCK_MONOTONIC);
    for (i=0; i<memfile_size; i+=PAGE_SIZE)
    {
        START_CLOCK(clk, CLOCK_MONOTONIC);
        tmp += *(volatile uint64_t *)(memfile + i);
        END_CLOCK(clk);

        if (i>0 && i % (1<<20)==0) {
            du.update();
            cout << dec << (i>>20) << "MB, " << (du.usage / 10.0) << "%, "
                 << clk.ns << " ns, " << du.clk_total.ns/1000000 << " ms" << endl;
            cout.flush();
       }
    }
    END_CLOCK(cl2);
    du.update();
    cout << "Size: " << dec << (memfile_size>>20) << "M, Time: " << (cl2.ns / 1000000) << " ms" << endl;
    close(fd);
    munmap(memfile, memfile_size);
}

int main(void)
{
    vanilla_waylaying();
    return 0;
}

