#include "lff.h"
using namespace std;

// util functions
void make_eviction_list(const vector<void *> &arr)
{
    for (int i=0; i<arr.size()-1; ++i)
        *((uint64_t *)arr[i]) = (uint64_t)arr[i+1];
    *((uint64_t *)arr.back()) = 0;
}

inline void access_eviction_list(void *start)
{
    register uint64_t a = (uint64_t)start;
    while ((a = *(uint64_t *)a);    
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

int lff_probe(vector<void *> &seq, void *cand)
{
    register uint64_t a;
    int ret = 999;
    
    make_eviction_list(seq);   // prepare linked list
    
    while (ret > MEM_THRESHOLD)
    {
        // visit cand first
        a = (uint64_t)cand; MOV(a);
        
        // eviction primitive
        a = (uint64_t)seq[0];
        while ((a = *(uint64_t *)a));
        
        // measure
        a = (uint64_t)cand;
        START_TSC_LT;
        MOV(a);
        END_TSC_LT;
        ret = _tsc;
    }
    return ret;
}

bool lff_is_conflict(vector<void *> &seq, void *cand)
{
    return seq.empty() ? false : lff_probe(seq, cand) > L3_THRESHOLD;
}

// similar to std::set_intersection, va/vb should be sorted
bool lff_is_intersect(vector<void *> &va, vector<void *> &vb)
{
    int i=0, j=0;
    while (i < va.size() && j < vb.size())
    {
        if (va[i] < vb[j]) ++i;
        else if (vb[j] < va[i]) ++j;
        else return true;
    }
    return false;
}

// merge vb into (->) va. va gets bigger. va/vb should be sorted
void lff_union(vector<void *> &va, vector<void *> &vb)
{
    int i=0; j=0;
    while (j<vb.size())
    {
        if (i>=va.size())
        {
            va.push_back(vb[j]); ++i; ++j;
        }
        else
        {
            while (i<va.size() && va[i]<vb[j]) ++i;
            if (i<va.size())
            {
                if (va[i]>vb[j]) va.push_back(vb[j]);
                ++j;
            }
        }
    }
    sort(va.begin(), va.end());
}

void EvictionSet::lff_build(uint64_t base_pa, int cache_size_kb, int way, int slice, int line_size)
{
    vector<uint64_t> off = build_offset(cache_size_kb, way, slice, size);
    vector<void *> lines, conflict_set;
    int i;
    void *l;
    
    eset.clear();
    this->cache_size_kb = cache_size_kb;
    this->way = way;
    this->slice = slice;
    this->line_size = line_size;
        
    // build initial conflict_set and lines set
    for (uint64_t x : off)
    {
        l = addrmap.p2v(base_pa + x);
        if (l)
        {
            if (!lff_is_conflict(conflict_set, l))
                conflict_set.push_back(l);
            else lines.push_back(l);
        }
    }
    
    // cluster other items into slices
    for (auto l : lines)
    {
        if (lff_is_conflict(conflict_set, l)    // this should always be true
        {
            vector<void *> sl; 
            sl.push_back(l);
            for (auto c : conflict_set)
            {
                // tmp = conflict_set - [c]
                vector<void *> tmp;
                for (auto t : conflict_set)
                    if (t!=c) tmp.push_back(t);
                
                // if without c, conflict disappears, then l and c are in the same slice
                if (!lff_is_conflict(tmp, l))
                    sl.push_back(c);
            }
            
            // merge sl into eset
            sort(sl.begin(), sl.end());
            for (i=0; i<eset.size(); ++i)
                if (lff_is_intersect(eset[i], sl))
                {
                    lff_union(eset[i], sl); break;
                }
                    
            // if not found, add new slice
            if (i>=eset.size())
                eset.push_back(sl);
        }
    }
    
    if (eset.size() > 0)
        this->cache_set = cache_set(addrmap.v2p(eset[0][0]));
}

void EvictionSet::change_cache_set(uint64_t pb)
{
    this->cache_set = cache_set(pb);
    cout << "EvictionSet: change set to #" << this->cache_set << endl;
    for (auto s : eset)
        for (int i=0; i<s.size(); ++i)
        {
            s[i] = change_cache_set_va(s[i], pb);
        }
}

int EvictionSet::lff_test_slice(uint64_t p)
{
    void *va = addrmap.p2v(p);
    int ret = 0, max = 0; t;
    
    change_cache_set(p);
    
    for (int i=0; i<eset.size(); ++i)
    {
        t = lff_probe(eset[i], va);
        if (t > max)
        {
            ret = i; max = t;
        }
        cout << "slice #" << i << ": " << t;
    }
    return ret;
}

