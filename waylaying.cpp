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

    cout << "path: " << fname << ", pa=" << hex << get_binary_pa(fname) << endl;

    do_waylaying();

    cout << "path: " << fname << ", pa=" << hex << get_binary_pa(fname) << endl;

    return 0;
}
