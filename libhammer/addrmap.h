#ifndef _ADDRMAP_HPP
#define _ADDRMAP_HPP

#include "page.h"
using namespace std;

class Page;

class AddrMap
{
public:
    map<void *, uint64_t> v2p_map;
    map<uint64_t, void *> p2v_map;
public:

    void clear() { v2p_map.clear(); p2v_map.clear(); }
    void add(Page &pg);

    void add(vector<Page> &pageset);

    void *p2v(uint64_t p)
    {
        uint64_t offset = p & 0xfff;
        uint64_t base = p - offset;            
        if (p2v_map.count(base)) return p2v_map[base] + offset;
        else return 0;
    }

    uint64_t v2p(void *v)
    {
        uint64_t offset = v & 0xfff;
        uint64_t base = v - offset;
        if (v2p_map.count(base)) return v2p_map[base] + offset;
        else return 0;
    }
};

extern AddrMap addrmap;

#endif
