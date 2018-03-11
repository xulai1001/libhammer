#include "timing.h"

#ifndef _TIMING_VARS
#define _TIMING_VARS
clk_t clk_freq = -1;
#else
extern clk_t clk_freq;
#endif

// baseline test
clk_t clock_overhead(int type)
{
    struct myclock cl;
    clk_t i, sum=0;
    // warmup
    for (i=0; i<10; ++i)
    {
        START_CLOCK(cl, type);
        END_CLOCK(cl);
        sum += cl.ns;
    }
    sum = 0;
    // test
    for (i=0; i<100; ++i)
    {
        START_CLOCK(cl, type);
        END_CLOCK(cl);
        sum += cl.ns;
    }
    return sum/100;
}

clk_t tsc_overhead(void)
{
    struct myclock cl;
    clk_t i, sum=0;
    //warmup
    for (i=0; i<10; ++i)
    {
        START_TSC(cl);
        END_TSC(cl);
        sum += cl.ticks;
    }
    sum = 0;
    //test
    for (i=0; i<1000; ++i)
    {
        START_TSC(cl);
        END_TSC(cl);
        if (cl.ticks < 100)     // rule out big values
            sum += cl.ticks;
    }
    return sum/1000;
}

clk_t tsc_measure_freq(void)
{
    struct myclock cl;
//    printf("tsc_measure_freq...");
    START_TSC(cl);
    usleep(1000000);
    END_TSC(cl);
//    printf("%ld MHz(Mticks/sec)\n", cl.ticks / 1000000);
    return cl.ticks;
}

clk_t tsc_to_ns(clk_t ticks)
{
    if (clk_freq<0) clk_freq = tsc_measure_freq();
    return ticks * 1000000000 / clk_freq;    
}

