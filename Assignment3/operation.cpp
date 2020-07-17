//
// Created by 허진욱 on 2020/06/12.
//

#include "operation.h"

Operation::Operation(int start_cycle, string op, int arg) {
    this->start_cycle = start_cycle;
    this->op = op;
    this->arg = arg;
}