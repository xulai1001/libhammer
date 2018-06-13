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
    for (i=0; i<memfile_size; i+=PAGE_SIZE)
    {
        START_CLOCK(clk, CLOCK_MONOTONIC);
        tmp += *(volatile uint64_t *)(memfile + i);
        END_CLOCK(clk);

        if (i>((uint64_t)(memfile_size - 100) << 20) && i % (20 * (1<<20))==0) {
            du.update();
        }
        if (i % (100 * (1<<20)) == 0)
        {
            cout << "."; cout.flush();
        }
    }
    cout << endl;
    du.update();
    last_disk_load = du.usage;
    END_CLOCK(cl2);
    // test_mb, avail_mb, read_ns, last_disk_load, total_time, is_evict
    avail_mb = get_available_mem() / 1024000;
    binpa2 = get_binary_pa(binary_path, 0);
    // cout << hex << binpa << " " << binpa2 << endl;
    cout << "test_mb=" << dec << (memfile_size / 1024000)
         << ", avail_mb=" << avail_mb
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
    myclock clk;
    START_CLOCK(clk, CLOCK_MONOTONIC);
    relocate_fadvise(path);
    END_CLOCK(clk);
    uint64_t binpa = get_binary_pa(binary_path, 0);
    paset.insert(binpa);
    cout << "time=" << dec << ((clk.ns / 1000000) / 1000.0)
         << ", addr=" << hex << binpa
         << ", coverage=" << dec << paset.size()/256 << " / " << avail_mb
         << endl;
}

void chasing_test(const string &path)
{
    uint64_t i, step=0, pa;
    int fd, w, tmp;
    char *image;
    unsigned sz;
    struct stat st;
    const uint64_t free_mb = avail_mb;
    const int master_pgid = getpgid(0);

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
        // 2. fork
        if (fork() != 0)
        {
            // parent: relocate
            // any write to shared page will cause relocation, either from parent or child. so parent write is also OK.
            *(volatile uint64_t *)image = ++step;
            // insert into set
            pa = v2p_once(image);
            paset.insert(pa);
            // analyse
            cout << "step=" << dec << step
                 << ", image=" << hex << pa
                 << ", coverage=" << dec << paset.size()/256 << " / " << avail_mb << endl;
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
    const string pth = vanilla_path;
    avail_mb = get_available_mem() / 1024000;
    myclock clk;
    START_CLOCK(clk, CLOCK_MONOTONIC);
    for (int i = 0; i<10; ++i)
    {
        //fadvise_test(binary_path);
        //waylaying_test(pth);

    }
    chasing_test(binary_path);
    return 0;
}

