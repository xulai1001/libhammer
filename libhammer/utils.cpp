#include "utils.h"
#include "fstream"
#include "set"
using namespace std;

extern "C" {
    #include "sigsegv.h"
    #include "sys/wait.h"
}

// get mem/cpu info
uint64_t get_meminfo(const string &key)
{
    stringstream cmd, ss;
    uint64_t ret;
    cmd << "awk '$1 == \"" << key << ":\" { print $2 }' /proc/meminfo";
    ss << run_cmd(cmd.str().c_str()); ss >> ret;
    return ret * 1024;
}

uint64_t get_mem_size()
{
    struct sysinfo info;
    sysinfo(&info);
    return (size_t)info.totalram * (size_t)info.mem_unit;
}

uint64_t get_cached_mem()
{
    return get_meminfo("Cached");
}

uint64_t get_available_mem()
{
    return get_meminfo("MemAvailable");
}

void set_cpu_affinity(int x)
{
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(x, &mask);
    ASSERT((sched_setaffinity(0, sizeof(mask), &mask)) != -1);
}

string run_cmd(const char *cmd)
{
    static FILE* pipe;
    char buf[2048];
    stringstream ss;
    // cout << "cmd: " << cmd << endl;
    ASSERT(pipe = popen(cmd, "r"));
    while (!feof(pipe))
        if (fgets(buf, 2048, pipe) != 0)
            ss << buf;
    pclose(pipe);

    return ss.str();
}

string get_cpu_model()
{
    return run_cmd("ruby -e \"STDOUT.write \\`cat /proc/cpuinfo\\`.lines.map{|l| l.split(':')}.find{|x| x[0]['model name']}[1].chomp.split(' ')[2]\"");
}

// error handling stub
void continuation(void *a, void *b, void *c)
{
    printf("segfault at 0x%lx", (uint64_t)a);
    exit(-1);
}
/*
int handler(void *faddr, int s)
{
    sigsegv_leave_handler(&continuation, faddr, 0, 0);
    return 1;
}
*/

uint64_t get_binary_pa(const string &path, uint64_t offset)
{
    int fd;
    unsigned sz;
    struct stat st;
    void *image;
    uint64_t ret;

    ASSERT(-1 != (fd = open(path.c_str(), O_RDONLY)) );
    fstat(fd, &st);
    sz = st.st_size;

    ASSERT(0 != (image = mmap(0, sz, PROT_READ, MAP_PRIVATE, fd, 0)) );
    mlock(image, sz);
    ret = v2p(image+offset);

    munlock(image, sz);
    close(fd);
    munmap(image, sz);
    return ret;
}

void waylaying()
{
    uint64_t i, cached_ns, uncached_ns;
    char *memfile=0;
    int fd;
    uint64_t memfile_size;
    struct stat st;
    volatile uint64_t tmp = 0;
    myclock clk, cl2;

    // open eviction file
    ASSERT(-1 != (fd = open("/tmp/libhammer/disk/memfile", O_RDONLY | O_DIRECT)) );
    fstat(fd, &st);
    memfile_size = st.st_size;
    ASSERT(0 != (memfile = mmap(0, memfile_size, PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, 0)) );

    // cout << "- start eviction" << endl;
    START_CLOCK(cl2, CLOCK_MONOTONIC);
    for (i=0; i<memfile_size; i+=PAGE_SIZE)
    {
        START_CLOCK(clk, CLOCK_MONOTONIC);
        tmp += *(volatile uint64_t *)(memfile + i);
        END_CLOCK(clk);

        if (i>0 && i % (1<<26)==0) {
           if (clk.ns < 150)
                cout << "-";
           else if (clk.ns < 1000)
                cout << ".";
           else
                cout << "+";
           cout.flush();

        }

    }
    END_CLOCK(cl2);
    cout << endl;
    close(fd);
    munmap(memfile, memfile_size);
}

// use fadvise to do waylaying!
void relocate_fadvise(const string& path)
{
    int fbin;
    struct stat st;

    ASSERT(-1 != (fbin = open(path.c_str(), O_RDONLY)) );
    fstat(fbin, &st);
    ASSERT(0 == posix_fadvise(fbin, 0, st.st_size, POSIX_FADV_DONTNEED));
    close(fbin);
}

uint64_t to_mb(uint64_t b)
{
    return b / 1024000ull;
}

bool is_paddr_available(uint64_t pa)
{
    bool ret = false;
    uint64_t avail_size, pool_size, i, tmp=0;

    avail_size = get_available_mem();
    pool_size = (uint64_t)(avail_size * 0.9);
    pool_size -= pool_size % PAGE_SIZE;
//    cout << blue << "- Available mem: " << _mb(avail_size) << "M, pool size: " << _mb(pool_size) << "M." << endl;

    {
        vector<Page> pool = allocate_mb(to_mb(pool_size));

        for (Page pg : pool)
        {
            pg.get<uint64_t>(0) = pg.p; // access

            if (pg.p == pa)
            {
                cout << "- pa=" << hex << pg.p << " is available." << endl;
                ret = true;
                break;
            }
        }
    }
    if (!ret) cout << "- pa=" << hex << pa << " not available." << endl;

    return ret;
}

uint64_t v2p_once(void *v) {
    int fd_pgmap;

    if (getuid() != 0) return -1; // returns when not root

    ASSERT((fd_pgmap = open("/proc/self/pagemap", O_RDONLY)) > 0);
    uint64_t vir_page_idx = (uint64_t)v / PAGE_SIZE;      // 虚拟页号
    uint64_t page_offset = (uint64_t)v % PAGE_SIZE;       // 页内偏移
    uint64_t pfn_item_offset = vir_page_idx*sizeof(uint64_t);   // pagemap文件中对应虚拟页号的偏移

    // 读取pfn
    uint64_t pfn_item, pfn;
    ASSERT( lseek(fd_pgmap, pfn_item_offset, SEEK_SET) != -1 );
    ASSERT( read(fd_pgmap, &pfn_item, sizeof(uint64_t)) == sizeof(uint64_t) );
    pfn = pfn_item & PFN_MASK;              // 取低55位为物理页号

    close(fd_pgmap);
    return pfn * PAGE_SIZE + page_offset;
}

void interrupt(int sig)
{
    static bool invoked = false;
    int master_pgid = getpgid(0);

    if (!invoked)
    {
        invoked = true;
        cout << "** Interrupted, sending SIGINT to group... **" << endl;
        kill(-master_pgid, SIGINT);
    }
    else
        cout << "- interrupt() already invoked on main process" << endl;
    exit(0);
}

void interrupt_child(int sig)
{
    cout << "-- Interrupted. pid=" << dec << getpid() << endl;
    exit(0);
}

static set<uint64_t> paset;

ImageFile do_chasing(const string &path, uint64_t addr)
{
    uint64_t i, step=0, pa;
    int fd, w, tmp;
    char *image;
    unsigned sz;
    struct stat st;
    const uint64_t free_mb = get_available_mem() / 1024000;
    const int master_pgid = getpgid(0);

    ImageFile ret;
    ret.image = image;
    ret.name = path;
    ret.sz = sz;

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
            cout << "step=" << dec << step << ", image=" << hex << pa
                 << ", coverage=" << dec << paset.size()/256 << "M / " << free_mb << "M" << endl;
            // wait for child to exit
            //wait(&i);
            usleep(1000);
        }
        else
        {
            setpgid(0, master_pgid);
            signal(SIGINT, interrupt_child);

            char *buf = mmap(0, 8192000, PROT_READ |PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            int sum = 0;
            for (i=0; i<8192000; i+=PAGE_SIZE)
            {
                sum += *(volatile int *)(buf+i);
            }
            // child: wait & quit
            usleep(200000);
            munmap(buf, 8192000);
            break;
        }
    }

    // cleanup
    munmap(image, sz);
    close(fd);
    ret.image = 0;
    // cout << "- exit: step " << dec << step << " pid=" << getpid() << endl;

    return ret;
}

void DiskUsage::get_diskstat()
{
    stringstream cmd, ss;
    cmd << "awk '$3 == \"" << disk << "\" { print $13 }' /proc/diskstats";
    ss << run_cmd(cmd.str().c_str()); ss >> value;
}

void DiskUsage::init(string d)
{
    START_CLOCK(clk, CLOCK_MONOTONIC);
    START_CLOCK(clk_total, CLOCK_MONOTONIC);
    disk = d; usage = 0;
    get_diskstat();
}

void DiskUsage::update()
{
    uint64_t last_value = value, ms;
    get_diskstat();
    END_CLOCK(clk);
    END_CLOCK(clk_total);
    ms = clk.ns / 1000000;
    usage = (value - last_value) * 1000 / ms;
    START_CLOCK(clk, CLOCK_MONOTONIC);
}
