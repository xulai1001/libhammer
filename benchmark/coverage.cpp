#include "iostream"
#include "string"
#include "sstream"
#include "set"
#include "libhammer.h"

const string disk_name = "ext4";
const string binary_path = "../target";
const string vanilla_path = "memfile";
const string memway_path = "/tmp/libhammer/disk/memfile";
const int max_mb = 7300;

uint64_t avail_mb;
set<uint64_t> paset;

void waylaying_test(string path)
{
    uint64_t i, cached_ns, uncached_ns;
    char *memfile=0;
    int fd, read_ns=0, last_disk_load;
    uint64_t memfile_size, binpa, binpa2;
    struct stat st;
    volatile uint64_t tmp = 0;
    myclock clk, cl2;
    DiskUsage du;

    uint64_t mb = avail_mb;
    // open eviction file
    ASSERT(-1 != (fd = open(path.c_str(), O_RDONLY | O_DIRECT)) );
    fstat(fd, &st);
    memfile_size = st.st_size;
    ASSERT(0 != (memfile = mmap(0, memfile_size, PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, 0)) );

    // cout << "- start eviction" << endl;
    START_CLOCK(cl2, CLOCK_MONOTONIC);
    binpa = get_binary_pa(binary_path, 0);
    du.init(disk_name);
    cout << "- waylaying...";
    for (i=0; i<(mb<<20); i+=PAGE_SIZE)
    {
        START_CLOCK(clk, CLOCK_MONOTONIC);
        tmp += *(volatile uint64_t *)(memfile + i);
        END_CLOCK(clk);

        if (i == ((mb - 200) << 20)) {
            du.update();    // only collect last 100M
        }
        if (i % (100 * (1<<20)) == 0)
        {
            cout << "."; cout.flush();
        }
    }
    //cout << endl;
    du.update();
    last_disk_load = du.usage;
    END_CLOCK(cl2);
    // test_mb, avail_mb, read_ns, last_disk_load, total_time, is_evict
    avail_mb = get_available_mem() / 1024000;
    binpa2 = get_binary_pa(binary_path, 0);
    // cout << hex << binpa << " " << binpa2 << endl;
    cout << "avail_mb=" << dec << avail_mb
         << ", disk_load=" << (last_disk_load / 10.0)
         << ", time=" << ((cl2.ns / 1000000) / 1000.0)
         << ", addr=" << hex << binpa2
         << ", evict=" << (binpa != binpa2)
         << endl;
    close(fd);
    munmap(memfile, memfile_size);
}

void fadvise_test(const string path)
{
    static int step = 0;
    myclock clk;
    avail_mb = get_available_mem() / 1024000;
    START_CLOCK(clk, CLOCK_MONOTONIC);
    //relocate_fadvise(path);
    waylaying_test(memway_path);
    uint64_t binpa = get_binary_pa(binary_path, 0);
    paset.insert(binpa);
    END_CLOCK(clk);
    cout << "step=" << dec << ++step
         << ", time=" << dec << (clk.ns / 100000) / 10.0 << " ms"
         << ", coverage=" << dec << paset.size() << " / " << avail_mb * 256
         << ", addr=" << hex << binpa
         << endl;
}

void memway_test(const string path)
{
    static int step = 0;
    static int last_size, repeat=0;
    myclock clk;
    avail_mb = get_available_mem() / 1024000;
    START_CLOCK(clk, CLOCK_MONOTONIC);
    relocate_fadvise(path);
    uint64_t binpa = get_binary_pa(binary_path, 0);
    last_size = paset.size();
    paset.insert(binpa);
    if (paset.size() == last_size)
    {
        ++repeat;
        if (repeat > 6)
        {
            repeat = 0;
            waylaying_test(memway_path);
        }
    }
    else
        repeat = 0;
    END_CLOCK(clk);
    cout << "step=" << dec << ++step
         << ", time=" << dec << (clk.ns / 100000) / 10.0 << " ms"
         << ", coverage=" << dec << paset.size() << " / " << avail_mb * 256
         << ", addr=" << hex << binpa
         << endl;
}

void chasing_test(const string &path)
{
    uint64_t i, step=0, pa;
    int fd, w, tmp;
    char *image;
    unsigned sz;
    struct stat st;
    struct myclock clk;
    const uint64_t free_mb = avail_mb;
    const int master_pgid = getpgid(0);
    int last_size = 0, repeat = 0;

    // 1. mmap the file image with private and r/w access
    ASSERT(-1 != (fd = open(path.c_str(), O_RDONLY)) );
    fstat(fd, &st);
    sz = st.st_size;
    ASSERT(0 != (image = mmap(0, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0)) );
    // make sure the image is loaded into memory.
    *(volatile uint64_t *)image = 0;
    signal(SIGINT, interrupt);

    while (true)
    {
        START_CLOCK(clk, CLOCK_MONOTONIC);
        // 2. fork
        if (fork() != 0)
        {
            // parent: relocate
            // any write to shared page will cause relocation, either from parent or child. so parent write is also OK.
            *(volatile uint64_t *)image = ++step;
            // insert into set
            pa = v2p(image);
            last_size = paset.size();
            paset.insert(pa);
            if (paset.size() == last_size)
            {
                ++repeat;
                if (repeat > 6)
                {
                    repeat = 0;
                    waylaying_test(memway_path);
                }
            }
            else
                repeat = 0;

            END_CLOCK(clk);
            // analyse
            cout << "step=" << dec << step
                 << ", time=" << (clk.ns / 1000) << " us"
                 << ", coverage=" << dec << paset.size() << " / " << avail_mb*256
                 << ", addr=" << hex << pa
                 << endl;

            usleep(1000);
        }
        else
        {
            // child
            setpgid(0, master_pgid);
            signal(SIGINT, interrupt_child);
            /*
            char *buf = mmap(0, 8192000, PROT_READ |PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            int sum = 0;
            for (i=0; i<8192000; i+=PAGE_SIZE)
            {
                sum += *(volatile int *)(buf+i);
            }
            */
            // child: wait & quit
            usleep(200000);
            //munmap(buf, 8192000);
            break;
        }
    }

    // cleanup
    munmap(image, sz);
    close(fd);
}

int main(void)
{
    const string pth = memway_path;
    avail_mb = get_available_mem() / 1024000;

/*
    while (true)
    {
        //fadvise_test(binary_path);
        //waylaying_test(pth);
        //memway_test(binary_path);
    }
*/
    chasing_test(binary_path);
    return 0;
}

