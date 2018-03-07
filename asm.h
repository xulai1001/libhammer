#ifndef _ASM_H
#define _ASM_H

#define HAMMER(a, b) __asm__ __volatile__( \
    "movq (%0), %%rax \n\t" \
    "movq (%1), %%rax \n\t" \
    "clflush (%0) \n\t" \
    "clflush (%1) \n\t" \
    : \
    :"r"(a), "r"(b) \
    :"rax", "memory")

// r/w barrier
#define MFENCE __asm__ __volatile__(\
    "mfence \n\t" \
    : : :"memory")

// read barrier. (in this way?)
#define LFENCE __asm__ __volatile__(\
    "lfence \n\t" \
    : : :"memory")

// read access (64-bit)
#define MOV(a) __asm__ __volatile__( \
    "movq (%0), %%rax \n\t" \
    : :"r"(a):"rax")

#define MOV2(a, b) __asm__ __volatile__( \
    "movq (%0), %%rax \n\t" \
    "movq (%1), %%rax \n\t" \
    : :"r"(a), "r"(b):"rax")

#define MOV4(a, b, c, d) __asm__ __volatile__( \
    "movq (%0), %%rax \n\t" \
    "movq (%1), %%rax \n\t" \
    "movq (%2), %%rax \n\t" \
    "movq (%3), %%rax \n\t" \
    : :"r"(a), "r"(b), "r"(c), "r"(d):"rax")

#define CLFLUSH(a) __asm__ __volatile__(\
    "clflush (%0) \n\t" \
    : :"r"(a))

#define CLFLUSH2(a, b) __asm__ __volatile__(\
    "clflush (%0) \n\t" \
    "clflush (%1) \n\t" \
    : :"r"(a), "r"(b))

#define PREFETCH_L2(a) __asm__ __volatile__(\
    "prefetcht1 (%0) \n\t" \
    : :"r"(a))

#define PREFETCH_L3(a) __asm__ __volatile__(\
    "prefetcht2 (%0) \n\t" \
    : :"r"(a))

#endif
