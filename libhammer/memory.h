#ifndef _MEMORY_H
#define _MEMORY_H

#include "asm.h"
#include "timing.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "errno.h"

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
    #define ASSERT(line) if (!(line)) { fprintf(stderr, "ASSERT error: " #line "\n%s\n", strerror(errno)); exit(-1); }
#endif

extern int fd_pagemap;

uint64_t v2p(void *v);

//---------------------------------
// hammer function. returns operation time (ticks)
uint64_t hammer_loop(void *va, void *vb, int n, int delay);
uint64_t hammer_loop_mfence(void *va, void *vb, int n, int delay);

// equivalent to usenix 16 obf latency routine
uint64_t hammer_latency(void *va, void *vb);

// March 11: add cpp interface
#define ROW_CONFLICT_THRESHOLD 233
int is_conflict(void *va, void *vb);

#endif
