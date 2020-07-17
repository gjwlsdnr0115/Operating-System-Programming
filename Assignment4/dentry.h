//
// Created by 허진욱 on 2020/06/26.
//

#ifndef HW4_DENTRY_H
#define HW4_DENTRY_H

#include <string>
#include <vector>
#include "inode.h"

using namespace std;

class Dentry {
public:
    string name;    //dentry name

    Dentry *parent; //parent dentry
    vector<Dentry*> d_dentry;   //sub dentries
    vector<Inode*> d_inode; //files in dentry

    Dentry(string name, Dentry* parent);    //constructor
};


#endif //HW4_DENTRY_H
