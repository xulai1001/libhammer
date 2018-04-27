#ifndef _PAGE_HPP
#define _PAGE_HPP

#include "libhammer.h"
using namespace std;

class Page
{
public:
    char *ptr;
    shared_ptr<char> v;
    uint64_t p, shmid;
    bool locked;
    static uint64_t shm_index, release_count;

private:
    // invoked by shared_ptr deleter
    static void _release(char *p);

public:

    Page() : p(0), shmid(0), locked(false) {}

    ~Page();

    void acquire();
    void acquire_shared(uint64_t sid=0);
    void reset();       // explicitly release a page
    void lock();
    void unlock();
    bool operator<(Page &b);
    bool operator==(Page &b);
    string inspect();

    template <typename T>
    T & get(size_t x)
    {
        return *(T *)(v.get()+x);
    }

    void fill(uint8_t x=0xff)
    {
        memset(v.get(), x, PAGE_SIZE);
    }

    void wrap()
    {
        v = shared_ptr<char>(ptr, this->_release);
        //v.reset(ptr, this->_release);
    }

    vector<int> check_bug(uint8_t good=0xff);

};

// page allocation
vector<Page> allocate_mb(int mb);
vector<Page> get_contiguous_aligned_page(vector<Page> & pageset);
vector<Page> allocate_cap(int pageset_mb);
void release_pageset(vector<Page> & pageset);

#endif
