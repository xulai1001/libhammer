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
    map<uint64_t, Page> page_map;
public:

    void clear() { v2p_map.clear(); p2v_map.clear(); page_map.clear(); }
    void add(Page &pg);
    void add(vector<Page> &pageset);
    void add_pagemap(vector<Page> &pageset);    // use with caution. for this will add reference to shared_ptr

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

    bool has_pa(uint64_t pa)
    {
        return (bool)p2v_map.count(pa);
    }
    bool has_va(void *va)
    {
        return (bool)v2p_map.count(va);
    }
};

extern AddrMap addrmap;

#endif
