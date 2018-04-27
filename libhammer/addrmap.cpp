#include "addrmap.h"
using namespace std;

AddrMap addrmap;

void AddrMap::add(Page &pg)
{
    v2p_map[pg.v.get()] = pg.p;
    p2v_map[pg.p] = pg.v.get();
    //page_map[pg.p] = pg;
}

void AddrMap::add(vector<Page> &pageset)
{
    for (auto pg : pageset) add(pg);
}

void AddrMap::add_pagemap(vector<Page> &pageset)
{
    for (auto pg : pageset)
        page_map[pg.p] = pg;
}
