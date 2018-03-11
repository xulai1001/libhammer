#ifndef _TIMING_H
#define _TIMING_H

#include "time.h"
//#include "stdio.h"
#include "unistd.h"
#include "linux/types.h"
#include "stdint.h"

typedef uint64_t clk_t;

struct myclock {
    // clock_gettime vars
    clk_t ns;
    int type;
    struct timespec t0, t1;
    // rdtsc vars
    clk_t ticks, r0, r1;
};

extern clk_t clk_freq;

#define START_CLOCK(cl, tp) cl.type = tp; clock_gettime(cl.type, &cl.t0)
#define END_CLOCK(cl) clock_gettime(cl.type, &cl.t1); \
    cl.ns = (cl.t1.tv_sec - cl.t0.tv_sec) * 1000000000 + (cl.t1.tv_nsec - cl.t0.tv_nsec)

// intel guide
// use cpuid as barrier before, cpuid will clobber(use) all registers
// shl/or joins eax/edx to 64-bit long
// no output vars
#define START_TSC(cl) __asm__ __volatile__ ( \
   "cpuid \n\
    rdtsc \n\
    shlq $32, %%rdx \n\
    orq %%rdx, %%rax" \
    : "=a"(cl.r0) \
    : \
    : "%rbx", "%rcx", "%rdx")

// shl/or joins eax/edx, then mov to var
// use cpuid as barrier after, will clobber all registers
// no output vars
// =g lets gcc decide howto deal with var
#define END_TSC(cl) __asm__ __volatile__ ( \
   "rdtscp \n\
    shlq $32, %%rdx \n\
    orq %%rdx, %%rax \n\
    movq %%rax, %0 \n\
    cpuid" \
    : "=g"(cl.r1) \
    : \
    : "%rax", "%rbx", "%rcx", "%rdx"); \
    cl.ticks = cl.r1 - cl.r0

// lightweight rdtsc
// reg is a 32-bit register var, used to store delta ticks
#define START_TSC_LT register int _tsc asm ("r12"); \
                     __asm__ __volatile__ ( \
    "lfence \n\t" \
    "rdtsc \n\t" \
    "movl %%eax, %0" \
    :"=r"(_tsc)::"%rax", "%rdx")

#define END_TSC_LT __asm__ __volatile__ (\
    "lfence \n\t" \
    "rdtsc \n\t" \
    "subl %0, %%eax \n\t" \
    "movl %%eax, %0" \
    :"=r"(_tsc)::"%rax", "%rdx")

// baseline test
clk_t clock_overhead(int type);
clk_t tsc_overhead(void);
clk_t tsc_measure_freq(void);
clk_t tsc_to_ns(clk_t ticks);
#endif
