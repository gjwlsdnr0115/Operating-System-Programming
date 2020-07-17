//
// Created by 허진욱 on 2020/06/12.
//

#ifndef HW3_P_OPERATION_H
#define HW3_P_OPERATION_H


#include <string>

using namespace std;

class Operation {
public:
    int start_cycle;
    string op;
    int arg;

    Operation(int start_cycle, string op, int arg);

};


#endif //HW3_P_OPERATION_H
