#include "memory.h"
#include "fcntl.h"

int fd_pagemap = -1;

// note: before calling v2p, v should be in memory
uint64_t v2p(void *v) {
    if (getuid() != 0) return -1; // returns when not root

    if (fd_pagemap < 0) ASSERT((fd_pagemap = open("/proc/self/pagemap", O_RDONLY)) > 0);
    uint64_t vir_page_idx = (uint64_t)v / PAGE_SIZE;      // 虚拟页号
    uint64_t page_offset = (uint64_t)v % PAGE_SIZE;       // 页内偏移
    uint64_t pfn_item_offset = vir_page_idx*sizeof(uint64_t);   // pagemap文件中对应虚拟页号的偏移

    // 读取pfn
    uint64_t pfn_item, pfn;
    ASSERT( lseek(fd_pagemap, pfn_item_offset, SEEK_SET) != -1 );
    ASSERT( read(fd_pagemap, &pfn_item, sizeof(uint64_t)) == sizeof(uint64_t) );
    pfn = pfn_item & PFN_MASK;              // 取低55位为物理页号
    return pfn * PAGE_SIZE + page_offset;
}

//---------------------------------
// hammer function. returns operation time (ticks)
uint64_t hammer_loop(void *va, void *vb, int n, int delay)
{
    struct myclock clk;
    register int i = n, j;

    START_TSC(clk);
    while (i--) {
        j = delay;
        HAMMER(va, vb);
        while (j-- > 0);
    }
    END_TSC(clk);

    return clk.ticks;
}

uint64_t hammer_loop_mfence(void *va, void *vb, int n, int delay)
{
    struct myclock clk;
    register int i = n, j;

    START_TSC(clk);
    while (i--) {
        j = delay;
        HAMMER(va, vb);
        MFENCE;
        while (j-- > 0);
    }
    END_TSC(clk);

    return clk.ticks;
}

// equivalent to usenix 16 obf latency routine
// test 10 times & return min value
uint64_t hammer_latency(void *va, void *vb)
{
    unsigned min = 999, n=3, tmp;
    while (min > 400)
    {
        n=3;
        while (n-- > 0)
        {
            tmp = hammer_loop_mfence(va, vb, 3, 0) / (3*2);
            tmp = hammer_loop_mfence(va, vb, 3, 0) / (3*2);
            if (tmp<min) min = tmp;
        }
    }
    // printf("%u ", min);
    return min;
}

#define ROW_CONFLICT_THRESHOLD 233
int is_conflict(void *va, void *vb)
{
    return hammer_latency(va, vb) >= ROW_CONFLICT_THRESHOLD;
}

