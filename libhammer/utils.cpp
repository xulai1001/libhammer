#include "utils.h"
#include "fstream"
#include "set"
using namespace std;

extern "C" {
    #include "sigsegv.h"
    #include "sys/wait.h"
}

// get mem/cpu info
uint64_t get_mem_size()
{
    struct sysinfo info;
    sysinfo(&info);
    return (size_t)info.totalram * (size_t)info.mem_unit;
}

uint64_t get_cached_mem()
{
    stringstream ss;
    uint64_t ret;
    ss << run_cmd("awk '$1 == \"Cached:\" { print $2 * 1024 }' /proc/meminfo");
    ss >> ret;
    return ret;
}

uint64_t get_available_mem()
{
    stringstream ss;
    uint64_t ret;
    ss << run_cmd("awk '$1 == \"MemAvailable:\" { print $2 * 1024 }' /proc/meminfo");
    ss >> ret;
    return ret;
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
    FILE* pipe;
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

uint64_t get_binary_pa(const string &path)
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
    ret = v2p(image);

    close(fd);
    munmap(image, sz);
    return ret;
}

void do_waylaying()
{
    uint64_t i, mem_size = get_available_mem();
    char *memfile;
    int fd;
    volatile uint64_t tmp = 0;

    cout << "- available mem size: " << dec << (mem_size / 1024000) << " M" << endl;
    ASSERT(-1 != (fd = open("memfile", O_RDONLY)) );
    ASSERT(0 != (memfile = mmap(0, mem_size+PAGE_SIZE, PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, 0)) );

    cout << "- start eviction" << endl;
    for (i=0; i<mem_size*0.9; i+=PAGE_SIZE)
    {
        tmp += (uint8_t)memfile[i];

        if (i>0 && i % (1<<24)==0) { cout << "."; cout.flush(); }
        if (i>0 && i % (1<<30)==0) cout << dec <<(i / 1024000000) << "G" << endl;
    }
    cout << endl; cout.flush();

    close(fd);
    munmap(memfile, mem_size+PAGE_SIZE);
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
