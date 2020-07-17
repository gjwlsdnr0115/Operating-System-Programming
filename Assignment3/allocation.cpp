//
// Created by 허진욱 on 2020/06/12.
//

#include "allocation.h"
#include <math.h>

using namespace std;

Allocation::Allocation(int size, int pt_start, int aid) {
    this -> size = size;
    this ->pt_start = pt_start;
    this -> aid = aid;
    in_pm_cycle = 0;
    in_pm = false;
    pm_start = -1;
    alloced = true;
    last_used = 0;
    r_bit = 0;
    accessed_count = 0;
    next_access = -1;

    r_byte = new int[8]();
    byte_value = 0;

    bool more = true;
    int count = 0;
    while(more) {
        if(size <= pow(2, count))
            more = false;
        else
            count++;
    }
    alloc_size = pow(2, count);
}
