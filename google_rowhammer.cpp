#include "libhammer.h"

using namespace std;

#define ROW_SHIFT 17
#define PAGES_PER_ROW 32

vector<Page> pages;
map<int, vector<Page> > pool;

void test_double_sided_rh()
{
    vector<int> test_rows;
    HammerResult rst;
    struct myclock clk;
    int cnt=0;

    for (auto it : pool)
        if (pool.count(it.first-1) && pool.count(it.first+1))
            test_rows.push_back(it.first);
    cerr << "testing " << test_rows.size() << " rows" << endl;
    rst.print_header();
    START_CLOCK(clk, CLOCK_MONOTONIC);
    for (int row : test_rows)
    {

        cerr << "- row " << dec << row << " (" << ++cnt << " / " << test_rows.size()
             << ") Elapsed: " << (clk.t1.tv_sec - clk.t0.tv_sec) << "s." << endl;

        // create page patterns
        for (Page p : pool[row-1])
            p.fill(0x55);
        for (Page q : pool[row+1])
            q.fill(0xaa);

        for (Page p : pool[row-1])
            for (Page q : pool[row+1])
            {
 //               cout << "here " << p.inspect() << q.inspect() << endl; cout.flush();
                // only check when there is row conflict
                if (is_conflict(p.v.get(), q.v.get()))
                {
                    vector<vector<int> > result_10, result_01;
                    result_10.clear(); result_01.clear();
                    map<uint64_t, uint8_t> flip_values; flip_values.clear();
                    int flips_row, t = 0; flips_row = 0;

                    // test 1-0 flip
                    // fill row with 0xff
                    for (Page r : pool[row])
                        r.fill();
                    hammer_loop(p.v.get(), q.v.get(), 1024000, 0);
                    for (Page r : pool[row])
                    {
                        result_10.push_back(r.check_bug());
                        // snapshot flipped values
                        for (int i : result_10.back())
                            flip_values[r.p+i] = r.get<uint8_t>(i);
                    }

                    // test 0-1 flip
                    // fill row with 0
                    for (Page r : pool[row])
                        r.fill(0);
                    hammer_loop(p.v.get(), q.v.get(), 1024000, 0);
                    for (Page r : pool[row])
                    {
                        result_01.push_back(r.check_bug(0));
                    }

                    for (auto res : result_10)
                        flips_row += res.size();
                    for (auto res : result_01)
                        flips_row += res.size();

                    rst.p = p.p;
                    rst.q = q.p;
                    rst.flips_row = flips_row;

                    for (t=0; t<result_10.size(); ++t)
                    {
                        rst.base = pool[row][t].p;
                        for (int i : result_10[t])
                        {
                            // cout << "+" << i << "=" << (int)r.get<uint8_t>(i) << " ";
                            rst.offset = i;
                            rst.value = (unsigned)flip_values[rst.base+i];
                            rst.flip_to = 0;
                            rst.flips_page = result_10[t].size();
                            rst.print();
                        }
                    }
                    for (t=0; t<result_01.size(); ++t)
                    {
                        rst.base = pool[row][t].p;
                        for (int i : result_01[t])
                        {
                            // cout << "+" << i << "=" << (int)r.get<uint8_t>(i) << " ";
                            rst.offset = i;
                            rst.value = (unsigned)pool[row][t].get<uint8_t>(i);
                            rst.flip_to = 1;
                            rst.flips_page = result_01[t].size();
                            rst.print();
                        }
                    }
                    cerr << "."; cout.flush();
                }
            }
        cerr << endl;
        END_CLOCK(clk);
    }
}

int main(int argc, char **argv)
{
    int i, j, mb=2048;

    // 1. allocate memory
    ASSERT(getuid() == 0);
    if (argc>1) mb = atoi(argv[1]);

    pages = allocate_mb(mb);

    // populate pool
    for (auto pg : pages)
    {
        int row = pg.p >> ROW_SHIFT;
        if (!pool.count(row)) pool[row] = vector<Page>();
        pool[row].push_back(pg);
    }
    // release unused rows, cannot use iterator
    auto it = pool.begin();
    while(it != pool.end()){
        if (it->second.size() < PAGES_PER_ROW)
        {
            //release_pageset(it->second);
            //it->second.clear();
            it = pool.erase(it);    // erase returns next iterator to it
        }
        else ++it;
    }
    cerr << "- freed " << dec << Page::release_count << " pages" << endl;
    cerr << "- we have " << pool.size() << " rows" << endl;

    test_double_sided_rh();

    return 0;
}
