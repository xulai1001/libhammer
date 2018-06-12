#include "iostream"
#include "string"
#include "sstream"
#include "libhammer.h"

class DiskUsage
{
public:
    myclock clk;
    uint64_t value, usage;
    string disk;

private:
    void get_diskstat()
    {
        stringstream cmd, ss;
        cmd << "awk '$3 == \"" << disk << "\" { print $13 }' /proc/diskstats";
        ss << run_cmd(cmd.str().c_str()); ss >> value;
    }

public:
    void init(string d)
    {
        START_CLOCK(clk, CLOCK_MONOTONIC);
        disk = d; usage = 0;
        get_diskstat();
    }

    void update()
    {
        uint64_t last_value = value, ms;
        get_diskstat();
        END_CLOCK(clk);
        ms = clk.ns / 1000000;
        usage = (value - last_value) * 1000 / ms;
        START_CLOCK(clk, CLOCK_MONOTONIC);
    }
};


int main(void)
{
    DiskUsage du;
    du.init("sda4");
    while (true)
    {
        du.update();
        cout << "sda4: " << (du.usage / 10.0) << "%" << endl;
        sleep(1);
    }
    return 0;
}

