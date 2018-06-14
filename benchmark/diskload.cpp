#include "iostream"
#include "string"
#include "sstream"
#include "libhammer.h"

const string disk_name = "sda4";
const string binary_path = "../target";
const string vanilla_path = "memfile";
const string memway_path = "/tmp/libhammer/disk/memfile";
const int max_mb = 4200;
int avail_mb;
// test_mb, avail_mb, max_read_ns, last_disk_load, total_time, is_evict
void waylaying_test(string path, uint64_t mb, bool warmup)
{
    uint64_t i, cached_ns, uncached_ns;
    char *memfile=0;
    int fd, read_ns=0, last_disk_load;
    uint64_t memfile_size, binpa, binpa2;
    struct stat st;
    volatile uint64_t tmp = 0;
    myclock clk, cl2;
    DiskUsage du;

    // open eviction file
    ASSERT(-1 != (fd = open(path.c_str(), O_RDONLY | O_DIRECT)) );
    fstat(fd, &st);
    memfile_size = st.st_size;
    ASSERT(0 != (memfile = mmap(0, memfile_size, PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, 0)) );

    // cout << "- start eviction" << endl;
    START_CLOCK(cl2, CLOCK_MONOTONIC);
    if (warmup)
    {
        cout << "- warmup " << dec << mb << " MB";
        for (i=0; i<memfile_size; i+=PAGE_SIZE)
        {
            if (i > (mb << 20)) break;
            tmp += *(volatile uint64_t *)(memfile + i);
            if (i>0 && i % (50 * (1<<20))==0)
            {
                cout << "."; cout.flush();
            }
       }
       END_CLOCK(cl2);
       cout << ((cl2.ns / 1000000) / 1000.0) << " s" << endl;
    }
    else
    {
        binpa = get_binary_pa(binary_path, 0);
        du.init(disk_name);
        for (i=0; i<memfile_size; i+=PAGE_SIZE)
        {
            if (i > (mb << 20)) break;
            START_CLOCK(clk, CLOCK_MONOTONIC);
            tmp += *(volatile uint64_t *)(memfile + i);
            END_CLOCK(clk);

            if (i==((mb-50) << 20))
            {
                du.update();
                read_ns = 0;
            }
            if (i>((mb-10) << 20)) {
                if (clk.ns > read_ns) read_ns = clk.ns;
            }
        }
        //cout << endl;
        du.update();
        last_disk_load = du.usage;
        END_CLOCK(cl2);
        // test_mb, avail_mb, read_ns, last_disk_load, total_time, is_evict
        binpa2 = get_binary_pa(binary_path, 0);
        // cout << hex << binpa << " " << binpa2 << endl;
        cout << "test_mb=" << dec << mb
             << ", avail_mb=" << avail_mb
             << ", read_ns=" << read_ns
             << ", disk_load=" << (last_disk_load / 10.0)
             << ", time=" << ((cl2.ns / 1000000) / 1000.0)
             << ", evict=" << (binpa != binpa2)
             << endl;
    }
    close(fd);
    munmap(memfile, memfile_size);
}

int main(void)
{
    const string pth = vanilla_path;
    avail_mb = get_available_mem() / 1024000;
    int mb;

    waylaying_test(pth, max_mb, true);
    waylaying_test(pth, avail_mb - 100, true);
    waylaying_test(pth, avail_mb - 100, true);
    waylaying_test(pth, 200, true);
    for (mb = 50; mb < avail_mb-150; mb+=50)
    {
        for (int i=0; i<3; ++i)
            waylaying_test(pth, mb, false);
    }

    for (mb; mb < avail_mb + 100; mb+=10)
        for (int i=0; i<10; ++i)
        waylaying_test(pth, mb, false);
    return 0;
}

