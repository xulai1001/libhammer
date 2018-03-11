#ifndef _PAGE_HPP
#define _PAGE_HPP

#include "libhammer.h"

class Page
{
private:
    char *ptr;
public:
    shared_ptr<char> v;
    uint64_t p, shmid;
    static uint64_t shm_index, release_count;
    
private:
    // invoked by shared_ptr deleter
    static void _release(char *p);

public:

    Page() : p(0), shmid(0) {}
    
    ~Page();
    
    void acquire();
    void acquire_shared(uint64_t sid=0);
    void reset();       // explicitly release a page 
    bool operator<(Page &b);
    bool operator==(Page &b);
    string inspect();
    
    template <typename T>
    T & get(size_t x)
    {
        return *(T *)(v.get()+x);
    }
    
    void fill(uint8_t x)
    {
        memset(v.get(), x, PAGE_SIZE);
    }
    
    void wrap()
    {
        v.reset(ptr, this->_release);
    }
    
    vector<int> check_bug(uint8_t good);

};

// page allocation
vector<Page> allocate_mb(int mb);
vector<Page> get_contiguous_aligned_page(vector<Page> & pageset);
vector<Page> allocate_cap(int pageset_mb);
void release_pageset(vector<Page> & pageset);

#endif
