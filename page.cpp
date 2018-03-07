#include "iostream"
#include "sstream"
#include "cstdio"
#include "vector"
#include "map"
#include "string"
#include "memory"

extern "C" {
    #include "unistd.h"
    #include "errno.h"
    #include "stdint.h"
    #include "fcntl.h"
    #include "sys/mman.h"
    #include "time.h"
    #include "linux/types.h"
    #include "sys/stat.h"
    #include "sys/sysinfo.h"
    #include "sys/ipc.h"
    #include "sys/shm.h"
    #include "sys/types.h"
    #include "sched.h"
    #include "sigsegv.h"

    #include "timing.h"
    #include "asm.h"

    #define V2P_EXTERN
    int fd_pagemap = -1;

    #include "memory.h"
}

using namespace std;

class Page
{
public:
    shared_ptr<char> v;
    uint64_t p, shmid;
    static uint64_t shm_index, release_count;
    
private:
    // invoked by shared_ptr deleter
    static void release(char *p)
    {
        if (p)
        {
            printf("- release 0x%lx\n", (uint64_t)p);
            munmap(p, PAGE_SIZE);
            ++release_count;
        }
    }

public:

    Page() : p(0), shmid(0) {}
    
    ~Page()
    {
        cout << "dtor" << endl;
        if (shmid > 0 && v.use_count()<=1)   // ?
        {
            // if using acquire_shared and the page (v) has been released
            // remove shm file here
            char fname[2048];
            sprintf(fname, "/dev/shm/page_%ld", shmid);
            unlink(fname);
            printf("removing shm page file %s\n", fname);
        }
    }
    
    shared_ptr<char> & acquire()
    {
        char *page = (char *)mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE,
                                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        ASSERT(page != MAP_FAILED);
        ASSERT(mlock(page, PAGE_SIZE) != -1);
        
        v = shared_ptr<char>(page, this->release);  // wrap
        if (getuid()==0) p = v2p(page);             // get paddr when root
        printf("+ acquire v=0x%lx, p=0x%lx\n", (uint64_t)page, p);
                
        return v;
    }
    
    shared_ptr<char> & acquire_shared(uint64_t sid=0)
    {
        char fname[2048], *page_shared;
        int fd, unused;
        
        // build filename
        if (sid==0) sid = shm_index++;
        shmid = sid;        
        sprintf(fname, "/dev/shm/page_%ld", sid);

        // create/open shared page file
        ASSERT((fd = open(fname, O_CREAT | O_RDWR, 0666)) > 0);
        lseek(fd, PAGE_SIZE, SEEK_SET);
        unused = write(fd, "", 1);
        
        // mmap
        page_shared = (char *)mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        ASSERT(page_shared != MAP_FAILED);
        ASSERT(mlock(page_shared, PAGE_SIZE) != -1);
        
        // alter permissions
        close(fd);
        chmod(fname, 0666);
        
        v = shared_ptr<char>(page_shared, this->release);   // wrap
        if (getuid()==0) p = v2p(page_shared);              // get phys addr when root
        printf("+ acquire v=0x%lx, p=0x%lx, path=%s\n", (uint64_t)page_shared, p, fname);
        
        return v;
    }

};
uint64_t Page::shm_index = 1, Page::release_count = 0;

int main(void)
{
    vector<Page> pp;
    for (int i=0; i<10; ++i)
    {
        Page p;
        p.acquire_shared();
        pp.push_back(p);
    }
    cout << "freed " << Page::release_count << " pages" << endl;    
    return 0;
}

