#ifndef _PAGE_HPP
#define _PAGE_HPP

#include "libhammer.h"

class Page
{
public:
    shared_ptr<char> v;
    uint64_t p, shmid;
    static uint64_t shm_index, release_count;
    
private:
    // invoked by shared_ptr deleter
    static void release(char *p);

public:

    Page() : p(0), shmid(0) {}
    
    ~Page();
    
    shared_ptr<char> & acquire();
    shared_ptr<char> & acquire_shared(uint64_t sid=0);
    bool operator<(Page &b);
    string inspect();
    
    template <typename T>
    T get(size_t x)
    {
        return *(T *)(v.get()+x);
    }
    
    void fill(uint8_t x)
    {
        memset(v.get(), x, PAGE_SIZE);
    }
    
    vector<int> check_bug(uint8_t good);

};

#endif
