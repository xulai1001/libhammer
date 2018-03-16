#ifndef _LFF_H
#define _LFF_H

#include "vector"

#include "timing.h"
#include "asm.h"
#include "page.h"
#include "addrmap.h"

#define L3_THRESHOLD 100

void make_eviction_list(const vector<void *> &arr)
{
    for (int i=0; i<arr.size()-1; ++i)
        *((uint64_t *)arr[i]) = (uint64_t)arr[i+1];
    *((uint64_t *)arr.back()) = 0;
}

vector<uint64_t> build_offset(int cache_size_kb, int way, int slice, int n)
{
    vector<uint64_t> ret; ret.resize(n);
    int set_count = (cache_size_kb << 4) / way / slice;
    int set_order = 32 - __builtin_clz(set_count - 1);
    
    cout << "- cache: " << cache_size_kb << " kB, " << way << "-way, " << slice << " slices, "
         << set_count << " sets, " << set_order << " set-bits." << endl;
         
    for (int i=0; i<n; ++i) ret[i] = i << (set_order + 6);
}

inline int cache_set(uint64_t pa) { return (pa & 0x1ffc0) >> 6; }
inline uint64_t change_cache_set(uint64_t pa, uint64_t pb)
{
    return (pa & ~0x1ffc0) | (pb & 0x1ffc0);
}
void *change_cache_set_va(void *va, uint64_t pb)
{
    return addrmap.p2v(change_cache_set(addrmap.v2p(va), pb));
}

#endif
