#ifndef _MEMORY_H
#define _MEMORY_H

#include "asm.h"
#include "timing.h"
#include "stdint.h"

//---------------------------------
// page select
#define PAGE_SIZE 0x1000
#define PAGE_MASK 0xfff
#define PAGE_SHIFT 12
#define PAGE_FLAG 0
#define HUGE_SIZE 0x200000ull
#define HUGE_MASK 0x1fffffull
#define HUGE_SHIFT 21
#define HUGE_FLAG 0x40000
#define PFN_MASK ((1ull << 55) - 1)
#define ALLOC_SIZE PAGE_SIZE

//---------------------------------
// v2p with pagemap
#ifndef ASSERT
    #define ASSERT(line) if (!(line)) { fprintf(stderr, "ASSERT error: " #line); exit(-1); }
#endif
#ifndef V2P_EXTERN
extern int fd_pagemap;
#endif

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
    uint64_t min = 999, n=3, tmp;
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
    return min;
}

#endif
