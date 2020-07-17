//
// Created by 허진욱 on 2020/06/26.
//

#include "dentry.h"

Dentry::Dentry(string name, Dentry* parent) {
    this -> name = name;
    this ->parent = parent;
}