//
// Created by 허진욱 on 2020/06/12.
//

#include "process.h"
#include <deque>

using namespace std;
Process::Process(string dir, string file_n, int pid, int pt_size) {
    name = file_n;
    this -> file_n = dir + "/" + file_n;
    this -> pid = pid;
    this -> pt_size = pt_size;
    const char* file_name = file_n.c_str();
    ifstream file_in;
    file_in.open(file_name);
    file_in>>total_op;
    for(int i=0; i< total_op*2; i++) {
        int op;
        file_in>>op;
        codes.push_back(op);
    }
    file_in.close();
    pt_aid = new int[pt_size];
    pt_v = new int[pt_size];
    for(int i=0; i<pt_size; i++) {
        pt_aid[i] = -1;
        pt_v[i] = -1;
    }
    current_op = 0;
    sleep = 0;
    line = 0;
    o = 0;
    a = 0;
    burst = 0;
    burst_count = 0;
    S.push_back(5);
    io_wait = false;
    lock_wait = false;
    done = false;
    end = false;
}

bool Process::is_Finished() {
    return done;
}