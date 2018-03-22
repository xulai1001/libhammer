#ifndef _LFF_H
#define _LFF_H

#include "vector"
#include "random"
#include "algorithm"
#include "sstream"

#include "timing.h"
#include "asm.h"
#include "page.h"
#include "addrmap.h"

#define L3_THRESHOLD 100
#define MEM_THRESHOLD 450
#define SET_MASK 0x1ffc0    // 3M cache - 0ffc0, 6M cache - 1ffc0
#define HASH(x) (((uint64_t)x >> 16) & 0xffff)  // use bit 16-31 to mark address

// NOTE: to use LFF algorithm, the global AddrMap object must be populated first
//------------------
// eviction linked list
void            make_eviction_list(const vector<void *> &arr);
inline void     access_eviction_list(void *start);

// cache utils
vector<uint64_t> build_offset(int cache_size_kb, int way, int slice, int n);
inline int      cache_set(uint64_t pa);
inline uint64_t change_cache_set(uint64_t pa, uint64_t pb);
void *          change_cache_set_va(void *va, uint64_t pb);

// lff algorithm
int             lff_probe(const vector<void *> &seq, void *cand);
bool            lff_is_conflict(const vector<void *> &seq, void *cand);
// set operations
template <class T>
inline void     print_vector(const vector<void *> &v, const T& tag);
vector<void *>  exclude(const vector<void *> &seq, void *a=0, void *b=0);
bool            lff_is_intersect(const vector<void *> &va, const vector<void *> &vb); // similar to std::set_intersection, va/vb should be sorted
void            lff_union(vector<void *> &va, vector<void *> &vb); // merge vb into (->) va. va gets bigger. va/vb should be sorted

// eviction object
class EvictionSet
{
public:
    vector<vector<void *> > eset;
    int cache_size_kb, way, slice, line_size;
    int cache_set;

public:
    void lff_build(uint64_t base_pa, int cache_size_kb, int way, int slice, int line_size);
    void change_cache_set(uint64_t pb);
    void change_cache_set(void *vb);
    int lff_test_slice(uint64_t pa);
    int lff_test_slice(void *va);
    void test();

};

#endif
