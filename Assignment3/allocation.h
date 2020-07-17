//
// Created by 허진욱 on 2020/06/12.
//

#ifndef HW3_P_ALLOCATION_H
#define HW3_P_ALLOCATION_H

#include <deque>

using namespace std;

class Allocation {
public:
    int size;
    int alloc_size;
    int aid;
    int pt_start;
    int pm_start;
    int in_pm_cycle;
    int last_used;
    int accessed_count;
    int r_bit;
    int* r_byte;
    int byte_value;
    int next_access;
    bool in_pm;
    bool alloced;

    Allocation(int size, int pt_start, int aid);
};


#endif //HW3_P_ALLOCATION_H
