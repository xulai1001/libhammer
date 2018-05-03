#include "templating.h"

using namespace std;

void HammerResult::print_header()
{
    cout << "base,offset,p,q,value,flip_to,flips_page,flips_row" << endl;
}

void HammerResult::print()
{
    cout << "0x" << hex << base << "," << dec << offset
         << hex << ",0x" << p << ",0x" << q << ",0x" << value << "," << flip_to
         << "," << dec << flips_page << "," << flips_row << endl;
}

HammerResult::HammerResult(const string& s)
{
    stringstream ss; ss.clear();
    char c; //comma
    ss << s;
    ss >> hex >> base >> c >> dec >> offset >> c >> hex >> p >> c >> q >> c >> value >> c >> dec >> flip_to;
    if (!ss.eof())
        ss >> c >> dec >> flips_page >> c >> flips_row;
}

map<int, vector<HammerResult> > result_pool;

void load_hammer_result(const string& fname)
{
    ifstream ifs(fname);
    string str;
    int cnt = 0;

    result_pool.clear();

    getline(ifs, str);  // remove title line
    while (getline(ifs, str))
    {
        ++cnt;
        HammerResult result(str);
        // result.print();
        if (result_pool.count(result.offset) == 0)
            result_pool[result.offset] = vector<HammerResult>();
        result_pool[result.offset].push_back(result);
    }
    cerr << "- " << dec << cnt << " templates loaded." << endl;
}

vector<HammerResult> find_template(const BinaryInfo &info)
{
    int page_offset = info.offset & 0xfff;
    vector<HammerResult> ret;

    if (result_pool.count(page_offset) == 0)
    {
        cerr << "- can't find template with page offset=" << dec << page_offset << endl;
        return ret;
    }

    unsigned b = (info.flip_to == 0 ? 0xff : 0);
    for (HammerResult r : result_pool[page_offset])
    {
        r.print();
        if (r.flip_to == info.flip_to &&
            ((r.value ^ b) == (info.orig ^ info.target)))
            {
                ret.push_back(r);
                //is_paddr_available(r.base);
            }
    }
    cerr << "- found " << dec << ret.size() << " templates." << endl;
    return ret;
}

vector<HammerResult> find_flips(uint64_t p, uint64_t q)
{
    vector<HammerResult> ret;
    for (auto it : result_pool)
    {
        for (HammerResult hr : it.second)
            if (hr.p == p && hr.q == q)
                ret.push_back(hr);
    }
    return ret;
}
