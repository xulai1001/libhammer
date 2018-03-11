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

