#include "iostream"
#include "sstream"
#include "string"
#include "cstdlib"
#include "vector"
#include "../libhammer.h"

using namespace std;

const string green="\033[0;32m", blue="\033[1;34m", red="\033[1;31m", yellow="\033[0;33m", restore="\033[0m";

uint64_t _mb(uint64_t b)
{
    return b / 1024000ull;
}

int main(int argc, char **argv)
{
    uint64_t avail_size, pool_size, target_pa, i, tmp=0;
    bool not_found = true;
    Page target_page;
    stringstream ss; ss.clear();

    if (argc < 2)
    {
        cout << "usage: check_pa [paddr]" << endl;
        return -1;
    }
    ss << hex << argv[1]; ss >> target_pa;

    avail_size = get_available_mem();
    pool_size = (uint64_t)(avail_size * 0.9);
    pool_size -= pool_size % PAGE_SIZE;
    cout << blue << "- Available mem: " << _mb(avail_size) << "M, pool size: " << _mb(pool_size) << "M. target pa: 0x" << hex << target_pa
         << restore << endl;

    {
        vector<Page> pool;

        pool = allocate_mb(_mb(pool_size));

        for (Page pg : pool)
        {
            pg.get<uint64_t>(0) = pg.p; // access

            if (pg.p == target_pa)
            {
                not_found = false;
                target_page = pg;
                mlock(pg.v.get(), PAGE_SIZE);
                break;
            }
        }
        for (Page pg : pool)
            if (pg.p != target_pa) pg.reset();
    }

    if (not_found)
    {
        cout << yellow << "* pa=0x" << hex << target_pa << " is occupied." << restore << endl;
    }
    else
    {
        cout << blue << hex << "- va=0x" << (uint64_t)target_page.v.get() << " pa=0x" << target_page.p << endl;
        cout << green << "- allocating ramdisk..." << restore << endl;
        system("./create_memfile.sh");
        munlock(target_page.v.get(), PAGE_SIZE);
        cout << blue << "- Done. wait for 5s..." << restore << endl;
        usleep(5000000);
        cout << blue << hex << "- va=0x" << (uint64_t)target_page.v.get() << " pa=0x" << v2p(target_page.v.get()) << endl;
    }

    return not_found;
}
