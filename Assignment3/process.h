//
// Created by 허진욱 on 2020/06/12.
//

#ifndef HW3_P_PROCESS_H
#define HW3_P_PROCESS_H


#include <vector>
#include <list>
#include <fstream>
#include "allocation.h"
#include <deque>

using namespace std;

class Process {
public:
    string file_n;
    string name;
    int pid;
    int sleep;
    bool io_wait;
    bool lock_wait;
    int total_op;
    int current_op;
    bool done;
    bool end;
    int line;
    int o;
    int a;
    int burst;
    int burst_count;
    int pt_size;
    vector<double> S;
    vector<double> T;
    vector<Allocation*> allocations;
    deque<int> codes;
    int* pt_aid;
    int* pt_v;
    Process(string dir, string file_n, int pid, int pt_size);
    bool is_Finished();
};


#endif //HW3_P_PROCESS_H
