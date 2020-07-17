//
// Created by 허진욱 on 2020/06/26.
//

#ifndef HW4_INODE_H
#define HW4_INODE_H

#include <string>

using namespace std;

class Inode {
public:
    int id; //inode id
    string name;    //file name
    int size;   //file size in bytes

    int direct_block;   //num of direct blocks
    int single_block;   //num of single indirect blocks containing files
    int single_indirect;    //num of single indirect blocks containing pointers
    int double_block;   //num of double indirect blocks containing files
    int double_indirect;    //num of double indirect blocks containing pointers

    Inode(string name, int size, int id);   //constructor
    int total_blocks(); //return num of total blocks
};


#endif //HW4_INODE_H
