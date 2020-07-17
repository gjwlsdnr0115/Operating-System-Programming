//
// Created by 허진욱 on 2020/06/26.
//

#include "inode.h"
#include <cmath>

Inode::Inode(string name, int size, int id) {
    this -> name = name;
    this -> size = size;
    this -> id = id;

    int required_blocks = int(ceil(size / 1024.0)); //1 block == 1024 bytes

    if(required_blocks <= 12) { //use only directed blocks
        direct_block = required_blocks;
        single_block = 0;
        single_indirect = 0;
        double_block = 0;
        double_indirect = 0;
    }
    else if(required_blocks <= 268) {   //use up to single indirect blocks
        direct_block = 12;
        single_block = required_blocks - 12;
        single_indirect = 1;
        double_block = 0;
        double_indirect = 0;
    }
    else {  //use up to double indirect blocks
        direct_block = 12;
        single_block = 256;
        single_indirect = 1;
        double_block = required_blocks - 268;
        double_indirect = 1 + int(ceil(double_block / 256.0));
    }
}

int Inode::total_blocks() { //return total blocks
    int total = 0;
    total = direct_block + single_block + single_indirect + double_block + double_indirect;
    return total;
}