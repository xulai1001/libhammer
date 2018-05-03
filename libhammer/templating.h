#ifndef _TEMPLATING_H_
#define _TEMPLATING_H_

#include "libhammer.h"
#include "string"
#include "map"
#include "vector"
#include "fstream"
#include "sstream"

struct HammerResult
{
    uint64_t base, offset, p, q;
    unsigned value, flip_to, flips_page, flips_row;

    HammerResult() {};
    HammerResult(const string& s);
    void print_header();
    void print();
};

struct BinaryInfo
{
    string path;
    uint64_t offset;
    uint8_t orig, target, flip_to;
};

extern map<int, vector<HammerResult> > result_pool;

void load_hammer_result(const string& fname);
vector<HammerResult> find_template(const BinaryInfo &info);
vector<HammerResult> find_flips(uint64_t p, uint64_t q);

#endif
