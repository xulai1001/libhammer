#include "page.h"

using namespace std;

Page::~Page()
{
    // cout << "dtor" << endl;
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
void Page::release(char *p)
{
    if (p)
    {
        // printf("- release 0x%lx\n", (uint64_t)p);
        munmap(p, PAGE_SIZE);
        ++release_count;
    }
}

shared_ptr<char> & Page::acquire()
{
    char *page = (char *)mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE,
                                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    ASSERT(page != MAP_FAILED);
    ASSERT(mlock(page, PAGE_SIZE) != -1);
    
    v = shared_ptr<char>(page, this->release);  // wrap
    if (getuid()==0) p = v2p(page);             // get paddr when root
    // printf("+ acquire v=0x%lx, p=0x%lx\n", (uint64_t)page, p);
            
    return v;
}

shared_ptr<char> & Page::acquire_shared(uint64_t sid)
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
    // printf("+ acquire v=0x%lx, p=0x%lx, path=%s\n", (uint64_t)page_shared, p, fname);
    
    return v;
}

bool Page::operator<(Page &b)
{
    return (p>0 && b.p>0) ? p<b.p : (uint64_t)v.get() < (uint64_t)b.v.get();
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
    char *buf = v.get();
    for (int i=0; i<PAGE_SIZE; ++i)
        if (buf[i] != good) ret.push_back(i);
    return ret;
}

uint64_t Page::shm_index = 1, Page::release_count = 0;

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
    cout << "freed " << Page::release_count << " pages" << endl;
}

int main(void) { _test_page(); return 0; }

