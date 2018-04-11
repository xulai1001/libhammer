#include "libhammer.h"

using namespace std;

#define ROW_SHIFT 17
#define PAGES_PER_ROW 32

vector<Page> pages;
map<int, vector<Page> > pool;

struct HammerResult
{
    uint64_t base, offset, p, q;
    unsigned value, flip_to;

    void print_header()
    {
        cout << "base,offset,p,q,value,flip_to" << endl;
    }

    void print()
    {
        cout << "0x" << hex << base << "," << dec << offset
             << hex << ",0x" << p << ",0x" << q << ",0x" << value << "," << flip_to << endl;
    }
};

void test_double_sided_rh()
{
    vector<int> test_rows;
    HammerResult rst;

    for (auto it : pool)
        if (pool.count(it.first-1) && pool.count(it.first+1))
            test_rows.push_back(it.first);
    cerr << "testing " << test_rows.size() << " rows" << endl;
    rst.print_header();

    for (int row : test_rows)
    {
        cerr << "- row " << dec << row << endl;

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
                    // test 1-0 flip
                    // fill row with 0xff
                    for (Page r : pool[row])
                        r.fill();

                    // hammer!
                    hammer_loop(p.v.get(), q.v.get(), 1024000, 0);

                    // check result
                    for (Page r : pool[row])
                    {
                        auto result = r.check_bug();                        if (!result.empty())
                        {
                            /*cout << endl << hex
                                 << "p=0x" << p.p << " q=0x" << q.p
                                 << " base=0x" << r.p << " offsets ";
                                 */
                            for (int i : result)
                            {
                                // cout << "+" << i << "=" << (int)r.get<uint8_t>(i) << " ";
                                rst.base = r.p;
                                rst.offset = i;
                                rst.p = p.p;
                                rst.q = q.p;
                                rst.value = (unsigned)r.get<uint8_t>(i);
                                rst.flip_to = 0;
                                rst.print();
                            }
                        }
                    }

                    // test 0-1 flip
                    // fill row with 0xff
                    for (Page r : pool[row])
                        r.fill(0);

                    // hammer!
                    hammer_loop(p.v.get(), q.v.get(), 1024000, 0);

                    // check result
                    for (Page r : pool[row])
                    {
                        auto result = r.check_bug(0);
                        if (!result.empty())
                        {
                            for (int i : result)
                            {
                                // cout << "+" << i << "=" << (int)r.get<uint8_t>(i) << " ";
                                rst.base = r.p;
                                rst.offset = i;
                                rst.p = p.p;
                                rst.q = q.p;
                                rst.value = (unsigned)r.get<uint8_t>(i);
                                rst.flip_to = 1;
                                rst.print();
                            }
                        }
                    }

                    cerr << "."; cout.flush();
                }
            }
        cerr << endl;
    }
}

void test_single_sided_rh(int delta=-3)
{
    vector<int> test_rows;

    for (auto it : pool)
        if (pool.count(it.first+delta) && pool.count(it.first+1))
            test_rows.push_back(it.first);
    cout << "testing " << test_rows.size() << " rows" << endl;

    for (int row : test_rows)
    {
        cout << "- row " << dec << row;

        for (Page p : pool[row+delta])
            for (Page q : pool[row+1])
            {
 //               cout << "here " << p.inspect() << q.inspect() << endl; cout.flush();
                // only check when there is row conflict
                if (is_conflict(p.v.get(), q.v.get()))
                {
                    // fill row with 0xff
                    for (Page r : pool[row])
                        r.fill();

                    // hammer!
                    hammer_loop(p.v.get(), q.v.get(), 2*1024000, 0);

                    // check result
                    for (Page r : pool[row])
                    {
                        auto result = r.check_bug();
                        if (!result.empty())
                        {
                            cout << endl << hex
                                 << "p=0x" << p.p << " q=0x" << q.p
                                 << " base=0x" << r.p << " offsets ";
                            for (int i : result)
                                cout << "+" << i << "=" << (int)r.get<uint8_t>(i) << " ";
                        }
                    }
                    cout << "."; cout.flush();
                }
            }
        cout << endl;
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
    //test_single_sided_rh(-7);

    return 0;
}
