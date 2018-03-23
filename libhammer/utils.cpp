#include "utils.h"
using namespace std;

extern "C" {
    #include "sigsegv.h"
}

// get mem/cpu info
uint64_t get_mem_size()
{
    struct sysinfo info;
    sysinfo(&info);
    return (size_t)info.totalram * (size_t)info.mem_unit;
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
    uint64_t i, mem_size = get_mem_size();
    char *memfile;
    int fd;
    volatile uint64_t tmp = 0;

    cout << "- mem size: " << dec << (mem_size>>20) << " M" << endl;
    ASSERT(-1 != (fd = open("memfile", O_RDONLY)) );
    ASSERT(0 != (memfile = mmap(0, mem_size+PAGE_SIZE, PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, 0)) );

    cout << "- start eviction" << endl;
    for (i=0; i<mem_size; i+=PAGE_SIZE)
    {
        tmp += (uint8_t)memfile[i];

        if (i>0 && i % (1<<24)==0) { cout << "."; cout.flush(); }
        if (i>0 && i % (1<<30)==0) cout << dec <<(i>>30) << "G" << endl;
    }
    cout << endl; cout.flush();

    close(fd);
    munmap(memfile, mem_size+PAGE_SIZE);
}

