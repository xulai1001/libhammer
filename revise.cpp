#include "libhammer.h"

using namespace std;

#define ROW_SHIFT 17
#define PAGES_PER_ROW 32
const string green="\033[0;32m", blue="\033[1;34m", red="\033[1;31m", yellow="\033[0;33m", restore="\033[0m";

vector<Page> pages;
map<int, vector<Page> > pool;

vector<Page> alloc_pages()
{
    uint64_t avail_size, pool_size;
    vector<Page> ret;

    avail_size = get_available_mem();
    pool_size = (uint64_t)(avail_size * 0.9);
    pool_size -= pool_size % PAGE_SIZE;
    cout << blue << dec << "- Available mem: " << to_mb(avail_size) << "M, pool size: " << to_mb(pool_size) << "M." << restore << endl;

    ret = allocate_mb(to_mb(pool_size));
    return ret;
}

void test_hr(HammerResult & hr)
{
    int flips_page = 0, flips_row = 0;
    int row = hr.base >> ROW_SHIFT;
    uint8_t b = (hr.flip_to == 1 ? 0 : 0xff);
    Page p, q, r;

    if (!addrmap.page_map.count(hr.p) || !addrmap.page_map.count(hr.q) || !addrmap.page_map.count(hr.base))
    {
        cout << yellow << "* Cannot hold hammering pages " << hex << hr.p << " ->" << hr.base << "<- " << hr.q << ", skipping..." << endl;
        return;
    }
    p = addrmap.page_map[hr.p]; q = addrmap.page_map[hr.q]; r = addrmap.page_map[hr.base];
    p.fill(0x55); q.fill(0xaa);
    for (Page pg : pool[row])
        pg.fill(b);

    hammer_loop(p.v.get(), q.v.get(), 1200000, 0);

    flips_page = r.check_bug(b).size();
    for (Page pg : pool[row])
        flips_row += pg.check_bug(b).size();

    if (flips_page == 1)
    {
        cout << green << "Flips_Page: " << dec << flips_page << ", Flips_Row: " << flips_row << " | "; hr.print(); cout << restore;
    }
    else
    {
        cout << yellow << "Flips_Page: " << dec << flips_page << ", Flips_Row: " << flips_row << " | "; hr.print(); cout << restore;
    }
}

int main(int argc, char **argv)
{
    int i, j, mb=2048;
    int start = 0;
    stringstream ss;

    if (argc > 1)
    {
        ss << argv[1]; ss >> dec >> start;
    }

    // 1. allocate memory
    ASSERT(getuid() == 0);

    pages = alloc_pages();
    addrmap.add_pagemap(pages);
    load_hammer_result("hammer_result.csv");

    // populate pool
    for (auto pg : pages)
    {
        int row = pg.p >> ROW_SHIFT;
        if (!pool.count(row)) pool[row] = vector<Page>();
        pool[row].push_back(pg);
    }

    // 2. test each hammer result
    cout << blue << "- Start offset: " << dec << start << restore << endl;
    for (auto it : result_pool)
    {
        if (it.first != start) continue;
        cout << green << "- page_offset = " << dec << it.first << restore << endl;
        for (HammerResult hr : it.second)
            test_hr(hr);
    }
    return 0;
}
