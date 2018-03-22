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
        void *ret = 0;
        if (p2v_map.count(base)) ret = p2v_map[base] + offset;
       // cout << "p2v " << hex << p << " -> " << ret << endl;
        return ret;
    }

    uint64_t v2p(void *v)
    {
        uint64_t offset = (uint64_t)v & 0xfff;
        uint64_t base = v - offset, ret = 0;
        if (v2p_map.count((void *)base)) ret = v2p_map[(void *)base] + offset;
       // cout << "v2p " << hex << v << " -> " << ret << endl;
        return ret;
    }
};

extern AddrMap addrmap;

#endif
