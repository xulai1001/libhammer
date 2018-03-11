#ifndef _ADDRMAP_HPP
#define _ADDRMAP_HPP

#include "page.h"
using namespace std;

class Page;

class AddrMap
{
public:
    map<char *, uint64_t> v2p_map;
    map<uint64_t, char *> p2v_map;
public:

    void clear() { v2p_map.clear(); p2v_map.clear(); }
    void add(Page &pg);

    void add(vector<Page> &pageset);

    char *p2v(uint64_t p)
    {
        if (p2v_map.count(p)) return p2v_map[p];
        else return 0;
    }

    uint64_t v2p(char *v)
    {
        if (v2p_map.count(v)) return v2p_map[v];
        else return 0;
    }
};

#endif
