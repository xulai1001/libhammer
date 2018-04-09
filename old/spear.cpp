#include "libhammer.h"
#include "time.h"
#include "math.h"

using namespace std;

uint64_t target_pa;

int main(int argc, char **argv)
{
    int step = 0;
    uint64_t pa = 0, last_pa = 0, t;

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

    ImageFile img = do_chasing(fname, target_pa);

    return 0;
}
