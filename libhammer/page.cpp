#include "libhammer.h"

using namespace std;

// Page class implementations
//-----------------------------------------
Page::~Page()
{
    //cout << "dtor" << endl;
    if (shmid > 0 && v.use_count()<=1)   // ?
    {
        // if using acquire_shared and the page (v) has been released
        // remove shm file here
        char fname[2048];
        sprintf(fname, "/dev/shm/page_%ld", shmid);
        unlink(fname);
        // printf("removing shm page file %s\n", fname);
    }
}

// invoked by shared_ptr deleter
void Page::_release(char *ptr)
{
    if (ptr)
    {
        //printf("- release 0x%lx\n", (uint64_t)ptr);
        munmap(ptr, PAGE_SIZE);
        ++release_count;
    }
}


// explicitly release a page
void Page::reset()
{
    //cout << inspect() << endl;
    if (shmid > 0 && v.use_count()<=1)
    {
        char fname[2048];
        sprintf(fname, "/dev/shm/page_%ld", shmid);
        unlink(fname);
        //printf("removing shm page file %s\n", fname);
    }
    if (locked) unlock();
    v.reset();
}

void Page::acquire()
{
    ptr = (char *)mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE,
                                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    ASSERT(ptr != MAP_FAILED);
    ASSERT(mlock(ptr, PAGE_SIZE) != -1);

    //v.reset(page, this->_release);  // cannot wrap here, will disturb mmap
    if (getuid()==0) p = v2p(ptr);             // get paddr when root
    //printf("+ acquire v=0x%lx, p=0x%lx\n", (uint64_t)page, p);
}

void Page::acquire_shared(uint64_t sid)
{
    char fname[2048];
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
    ptr = (char *)mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ASSERT(ptr != MAP_FAILED);
    // ASSERT(mlock(ptr, PAGE_SIZE) != -1);

    // alter permissions
    close(fd);
    chmod(fname, 0666);

//    v.reset(page_shared, this->_release);   // cannot wrap here, will disturb mmap
    if (getuid()==0) p = v2p(ptr);              // get phys addr when root
    // printf("+ acquire v=0x%lx, p=0x%lx, path=%s\n", (uint64_t)ptr, p, fname);

}

void Page::lock()
{
    ASSERT(mlock(ptr, PAGE_SIZE) != -1);
    locked = true;
}

void Page::unlock()
{
    munlock(ptr, PAGE_SIZE);
    locked = false;
}

bool Page::operator<(Page &b)
{
    return (p>0 && b.p>0) ? p<b.p : (uint64_t)v.get() < (uint64_t)b.v.get();
}

bool Page::operator==(Page &b)
{
    return (p>0 && b.p>0) ? p == b.p : (uint64_t)v.get() == (uint64_t)b.v.get();
}

string Page::inspect()
{
    stringstream ss;
    ss << "<Page v=0x" << hex << (uint64_t)v.get() << " p=0x" << p;
    if (shmid>0) ss << " path=/dev/shm/page_" << dec << shmid;
    ss << dec << ">";
    return ss.str();
}

vector<int> Page::check_bug(uint8_t good)
{
    vector<int> ret;
    uint8_t *buf = (uint8_t *)v.get();
    for (int i=0; i<PAGE_SIZE; ++i)
        if (buf[i] != good) ret.push_back(i);
    return ret;
}

uint64_t Page::shm_index = 1, Page::release_count = 0;

// page allocator
//----------------------------
vector<Page> allocate_mb(int mb)
{
    vector<Page> ret;
    cerr << "- allocate "<< mb << "M memory" << endl;
    ret.resize(mb*256);

    for (int i=0; i<mb*256; ++i)
    {
        ret[i].acquire();
        // ret[i].get<int>(0) = i;  // write to the page
        if ((i+1) % (256*16) == 0) { cerr << "."; cerr.flush(); }  // 16MB/1G indicator
        if ((i+1) % (256*1024) == 0) cerr << endl;
    }
    for (int i=0; i<mb*256; ++i) ret[i].wrap();

    cerr << endl;
    sort(ret.begin(), ret.end());   // sort by paddr

    return ret;
}

vector<Page> get_contiguous_aligned_page(vector<Page> & pageset)
{
    vector<Page> ret;
    vector<int> chunk_length;
    int max_len=0, max_idx=0, i;
    int start_idx, st, order;

    chunk_length.resize(pageset.size());

    for (i=0; i<pageset.size(); ++i)
    {
        if (i>0 && pageset[i].p - pageset[i-1].p == PAGE_SIZE)
            chunk_length[i] = chunk_length[i-1] + 1;
        else chunk_length[i] = 1;

        if (chunk_length[i] > max_len)
        {
            max_len = chunk_length[i]; max_idx = i;
            // cout << hex << pageset[i].p << " " << dec << chunk_length[i] << endl;
        }
    }
    start_idx = max_idx - max_len + 1;
    order = 32 - __builtin_clz(max_len);
    st = -1;
    // cout << start_idx << " " << max_len << " " << order << endl;
    while (st<0 && order>0)
    {
        order--;
        for (i=start_idx; i<=max_idx - (1 << order)+1; ++i)
            if (__builtin_ffsll(pageset[i].p) >= order+PAGE_SHIFT+1)    // aligned to (order+12) bits
            {
                st = i; break;
            }
    }

    for (i=st; i<st+(1<<order); ++i)
        ret.push_back(pageset[i]);

    cerr << "- CAP is " << ret.size() << " pages " << ret.size()/256 << " MB, paddr "
         << hex << ret[0].p << "-" << ret.back().p+0xfff << endl;
    return ret;
}

vector<Page> allocate_cap(int pageset_mb)
{
    vector<Page> pool, ret;

    pool = allocate_mb(pageset_mb);
    ret = get_contiguous_aligned_page(pool);

    int count=0;
    for (auto p : pool)
        if (p < ret[0] || ret.back() < p)
        {
            ++count; p.reset();
        }
    cerr << dec << count << " unused page released." << endl;
    return ret;
}

void release_pageset(vector<Page> & pageset)
{
    for (auto p : pageset) p.reset();
}

// unit test
//----------------------------
void _test_alloc()
{
    auto pages = allocate_cap(1200);
    cout << "freed " << Page::release_count << " pages" << endl;
    release_pageset(pages);
    cout << "freed " << Page::release_count << " pages" << endl;
}

void _test_page()
{
    cout << get_cpu_model() << endl;
    vector<Page> pp;
    for (int i=0; i<10; ++i)
    {
        Page p;
        p.acquire_shared();
        cout << p.inspect() << endl;
        pp.push_back(p);
    }
    for (int i=0; i<10; ++i)
        pp[i].reset();
    cout << "freed " << dec << Page::release_count << " pages" << endl;
}

#ifdef UNIT_TEST
int main(void) {
    if (getuid()==0) _test_alloc();
    _test_page();
    return 0;
}
#endif

