#include "libhammer.h"

using namespace std;

vector<Page> pages;
EvictionSet eset;

int main(void)
{
    pages = allocate_cap(512);
    addrmap.add(pages);

    eset.lff_build(pages[0].p, 3072, 12, 4, 96);
    eset.test();

    release_pageset(pages);

    return 0;
}
