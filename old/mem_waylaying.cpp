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
    uint64_t i, j;
    uint64_t cached_ns, uncached_ns;
//    uint64_t cached_count = 0, uncached_count = 0, fast_count = 0;
    char *memfile=0, *image=0, c=0;
    int fd, fbin, sz, memfile_size, gb_total, gb_start, gb_end;
    struct stat st;
    volatile uint64_t tmp = 0;
    myclock clk, cl2;
    const uint64_t gb_size = PAGE_SIZE * 250000ull;

    // open eviction file
    ASSERT(-1 != (fd = open("/tmp/libhammer/disk/memfile", O_RDONLY | O_DIRECT)) );
    fstat(fd, &st);
    memfile_size = st.st_size;
    gb_total = memfile_size / 1024000000ull;
    ASSERT(0 != (memfile = mmap(0, memfile_size, PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, 0)) );
    // hold target binary
    ASSERT(-1 != (fbin = open(path.c_str(), O_RDONLY)) );
    fstat(fbin, &st);
    sz = st.st_size;

    ASSERT(0 != (image = mmap(0, sz, PROT_READ, MAP_PRIVATE, fbin, 0)) );
    ASSERT(image != -1);
    tmp += *(volatile uint64_t *)image;      // access

    tmp += *(volatile uint64_t *)memfile;      // access
    cout << "- memfile pa: " << hex << v2p(memfile) << endl;

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
    cout << "Evicted after " << dec << (j*1000+i/1024000) << "M, Time: " << (cl2.ns / 1000) / 1.0e6 << " s" << endl;

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

int main(int argc, char **argv)
{
    struct myclock myclk;

    if (argc<2)
    {
        cout << "usage: ./memway [binary]" << endl;
        return 1;
    }
    string fname(argv[1]);

    current_pa = get_binary_pa_offset(fname, 0);
    cout << "* current_pa = " << hex << current_pa << endl;
    //exit(0);

    while (true)
    {
        ++step;
        START_CLOCK(myclk, CLOCK_MONOTONIC);
        waylaying(fname, 1);
        current_pa = get_binary_pa_offset(fname, 0);
        END_CLOCK(myclk);

        cout << "+ current_pa = " << hex << current_pa
                 << " uptime: " << (myclk.t1.tv_sec - myclk.t0.tv_sec) << "s." << endl;
    }
    return 0;
}
