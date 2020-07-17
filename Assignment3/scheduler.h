//
// Created by 허진욱 on 2020/06/12.
//

#ifndef HW3_P_SCHEDULER_H
#define HW3_P_SCHEDULER_H

#include <queue>
#include <math.h>
#include <deque>
#include "allocation.h"
#include "algorithm.h"

using namespace std;

//check if all processes are finished
bool all_Finished(vector<Process*> v) {
    if(v.empty())
        return false;
    bool finished = true;
    for(int i=0; i<v.size(); i++) {
        if(!v[i]->is_Finished())
            finished = false;
    }
    return finished;
}

//returns process that matches the given PID
Process* get_process_from_pid(vector<Process*> &v, int p) {
    for(int i=0; i<v.size(); i++) {
        if(v[i]->pid == p && v[i]->io_wait == true)
            return v[i];
    }
    return nullptr;
}

//check if input variable is already locked by different process
bool locked(vector<int> &locks, int id) {
    bool lock = false;
    for(int i=0; i<locks.size(); i++) {
        if(locks[i] == id)
            lock = true;
    }
    return lock;
}

//returns the start index of available spot it PMemory
int get_pt_spot(int* pt, int size, int pt_size) {
    int idx = -1;
    int count = 0;
    for(int i=0; i<pt_size; i++) {
        if(pt[i] == -1) {
            count++;
            if(count == size) {
                idx = i - (count - 1);
                break;
            }
        }
        else {
            count = 0;
        }
    }
    //returns -1 if no spot
    return idx;
}

//for fcfs and rr
void operate(Process* p, int cycle, vector<int> &locks, vector<Process*> &processes, vector<Process*> &sleep_list, vector<Process*> &io_wait_list, int* p_memory, int PMSize, int &aid, vector<Allocation*> &all_allocs, int &page_fault, string page, int &time_interval) {
    vector<int>::iterator iter = locks.begin();
    int op = p->codes.front();      //get operation
    p->codes.pop_front();
    int arg = p->codes.front();     //get argument
    p->codes.pop_front();
    p->line++;      //increase current line
    p->o = op;
    p->a = arg;

    if(time_interval == 8) {    //if time interval
        for(int i=0; i<all_allocs.size(); i++) {
            if( all_allocs[i]->in_pm) {
                //put reference bit into reference byte
                for(int j=6; j >= 0; j-- ) {
                    all_allocs[i]->r_byte[j+1] = all_allocs[i]->r_byte[j];
                }
                all_allocs[i]->r_byte[0] = all_allocs[i]->r_bit;
                //reset reference bit
                all_allocs[i]->r_bit = 0;
            }
        }
        time_interval = 0;
    }

    switch (op) {
        case 0: {
            int idx = get_pt_spot(p->pt_aid, arg, p->pt_size);  //get index in page table
            Allocation* a = new Allocation(arg, idx, aid);  //make alloc object
            p->allocations.push_back(a);
            all_allocs.push_back(a);
            for(int i=0; i<arg; i++) {  //put in page table
                p->pt_aid[i+idx] = a->aid;
                p->pt_v[i+idx] = 0; //init as 0
            }
            aid++;  //increment AID
            break;
        }
        case 1: {
            Allocation* a;
            for(int i=0; i<all_allocs.size(); i++) {    //get alloc object
                if(all_allocs[i]->aid == arg) {
                    a = all_allocs[i];
                    break;
                }
            }
            a->accessed_count++;
            a->last_used = cycle;
            a->r_bit = 1;
            if(!a->in_pm) { //if not in physical memory
                page_fault++;   //increment page fault
                int idx = get_inPM_idx(a, p_memory, PMSize);    //get available index
                if(idx != -1) { //available
                    a->pm_start = idx;
                    allocate(a, p_memory, p->pt_v, cycle);
                }
                else {      //not available
                    if(page == "fifo")
                        a->pm_start = fifo(p_memory, PMSize, a, all_allocs, processes);
                    else if(page == "lru")
                        a->pm_start = lru(p_memory, PMSize, a, all_allocs, processes);
                    else if(page == "lru-sampled")
                        a->pm_start = s_lru(p_memory, PMSize, a, all_allocs, processes);
                    else if(page == "lfu")
                        a->pm_start = lfu(p_memory, PMSize, a, all_allocs, processes);
                    else if(page == "mfu")
                        a->pm_start = mfu(p_memory, PMSize, a, all_allocs, processes);
                    else
                        a->pm_start = optimal(p_memory, PMSize, a, all_allocs, processes);
                    allocate(a, p_memory, p->pt_v, cycle);
                }
            }
            break;
        }
        case 2: {
            for(int i=0; i<p->pt_size; i++) {   //remove from page table
                if(p->pt_aid[i] == arg) {
                    p->pt_aid[i] = -1;
                    p->pt_v[i] = -1;
                }
            }
            for(int i=0; i<p->allocations.size(); i++) {
                Allocation* a =p->allocations[i] ;
                if(a->aid == arg) {
                    a->alloced = false;
                    a->in_pm = false;
                    a->r_bit = 0;
                    for(int j=0; j<8; j++) {
                        a->r_byte[j] = 0;
                    }
                    a->byte_value = 0;
                    for(int j=0; j<PMSize; j++) {   //remove from PMemory
                        if(p_memory[j] == a->aid)
                            p_memory[j] = -1;
                    }
                    break;
                }
            }
            vector<Allocation*>::iterator itr;
            itr = all_allocs.begin();
            for(int i=0; i<all_allocs.size(); i++) {
                if(all_allocs[i]->aid == arg) {
                    itr += i;
                    all_allocs.erase(itr);
                    break;
                }
            }
            break;
        }
        case 3:
            break;
        case 4:
            if(p->codes.size() == 0) {
                p->done = true;
                p->end = true;
            }
            else{
                p->sleep = cycle + arg;
                p->end = true;
                sleep_list.push_back(p);

            }
            break;
        case 5:
            if(p->codes.size() == 0) {
                p->done = true;
                p->end = true;
            }
            else {
                p->io_wait = true;
                p->end = true;
                io_wait_list.push_back(p);
            }
            break;
        case 6:
            if(locked(locks, arg)){ //argument already locked
                p->codes.push_front(arg);   //restore arg
                p->codes.push_front(op);    //restore op
                p->lock_wait = true;        //busy waiting
            }
            else {
                locks.push_back(arg);
                p->lock_wait = false;
            }
            break;
        case 7:
            for(int i=0; i<locks.size(); i++) {
                if(locks[i] == arg) {
                    iter += i;
                    locks.erase(iter);
                }
            }
            break;
        default:
            cout<<"wrong"<<endl;
    }
    if(p->codes.size() == 0) {
        p->done = true;
        p->end = true;
    }
    time_interval++;
}

//simple estimation of cpu burst
double simple_estimate(vector<double> &v) {
    double sum = 0;
    for(int i=0; i<v.size(); i++) {
        sum += v[i];
    }
    return sum / v.size();
}

//exponential estimation of cpu burst
double exponential_estimate(vector<double> &v) {
    if(v.size() > 1) {
        double sum = 0;
        double a = 0.6;
        for(int i=0; i<v.size()-1; i++) {
            sum += a * pow((1-a), i) * v[v.size()-1-i];
        }
        sum += pow((1-a), v.size()-2) * v[0];
        return sum;
    }
    else {
        return v[0];
    }
}

//for sjf-simple
void operate_sjfs(Process* p, int cycle, vector<int> &locks, vector<Process*> &processes, vector<Process*> &sleep_list, vector<Process*> &io_wait_list, int* p_memory, int PMSize, int &aid, vector<Allocation*> &all_allocs, int &page_fault, string page, int &time_interval) {
    vector<int>::iterator iter = locks.begin();
    int op = p->codes.front();
    p->codes.pop_front();
    int arg = p->codes.front();
    p->codes.pop_front();
    p->line++;
    p->burst++;
    p->S[p->burst_count] -= 1;
    p->o = op;
    p->a = arg;

    if(time_interval == 8) {
        for(int i=0; i<all_allocs.size(); i++) {
            if( all_allocs[i]->in_pm) {
                for(int j=6; j >= 0; j-- ) {
                    all_allocs[i]->r_byte[j+1] = all_allocs[i]->r_byte[j];
                }
                all_allocs[i]->r_byte[0] = all_allocs[i]->r_bit;
                all_allocs[i]->r_bit = 0;
            }
        }
        time_interval = 0;
    }

    switch (op) {
        case 0: {
            int idx = get_pt_spot(p->pt_aid, arg, p->pt_size);
            Allocation* a = new Allocation(arg, idx, aid);
            p->allocations.push_back(a);
            all_allocs.push_back(a);
            for(int i=0; i<arg; i++) {
                p->pt_aid[i+idx] = a->aid;
                p->pt_v[i+idx] = 0;
            }
            aid++;
            break;
        }
        case 1: {
            Allocation* a;
            for(int i=0; i<all_allocs.size(); i++) {
                if(all_allocs[i]->aid == arg) {
                    a = all_allocs[i];
                    break;
                }
            }
            a->accessed_count++;
            a->last_used = cycle;
            a->r_bit = 1;
            if(!a->in_pm) {
                page_fault++;
                int idx = get_inPM_idx(a, p_memory, PMSize);
                if(idx != -1) {
                    a->pm_start = idx;
                    allocate(a, p_memory, p->pt_v, cycle);
                }
                else {
                    if(page == "fifo")
                        a->pm_start = fifo(p_memory, PMSize, a, all_allocs, processes);
                    else if(page == "lru")
                        a->pm_start = lru(p_memory, PMSize, a, all_allocs, processes);
                    else if(page == "lru-sampled")
                        a->pm_start = s_lru(p_memory, PMSize, a, all_allocs, processes);
                    else if(page == "lfu")
                        a->pm_start = lfu(p_memory, PMSize, a, all_allocs, processes);
                    else if(page == "mfu")
                        a->pm_start = mfu(p_memory, PMSize, a, all_allocs, processes);
                    else
                        a->pm_start = optimal(p_memory, PMSize, a, all_allocs, processes);
                    allocate(a, p_memory, p->pt_v, cycle);
                }
            }
            break;
        }
        case 2: {
            for(int i=0; i<p->pt_size; i++) {
                if(p->pt_aid[i] == arg) {
                    p->pt_aid[i] = -1;
                    p->pt_v[i] = -1;
                }
            }
            for(int i=0; i<p->allocations.size(); i++) {
                Allocation* a =p->allocations[i] ;
                if(a->aid == arg) {
                    a->alloced = false;
                    a->in_pm = false;
                    a->r_bit = 0;
                    for(int j=0; j<8; j++) {
                        a->r_byte[j] = 0;
                    }
                    a->byte_value = 0;
                    for(int j=0; j<PMSize; j++) {
                        if(p_memory[j] == a->aid)
                            p_memory[j] = -1;
                    }
                    break;
                }
            }
            vector<Allocation*>::iterator itr;
            itr = all_allocs.begin();
            for(int i=0; i<all_allocs.size(); i++) {
                if(all_allocs[i]->aid == arg) {
                    itr += i;
                    all_allocs.erase(itr);
                    break;
                }
            }
            break;
        }
        case 3:
            break;
        case 4:
            if(p->codes.size() == 0) {  //sleep is the final operation
                p->done = true;
                p->end = true;
            }
            else{
                p->sleep = cycle + arg;
                p->end = true;
                p->T.push_back(p->burst);   //push actual time of current burst
                p->burst = 0;
                double burst_estimate = simple_estimate(p->T);  //get estimation of next burst
                p->S.push_back(burst_estimate);
                p->burst_count++;
                sleep_list.push_back(p);

            }
            break;
        case 5: {
            if(p->codes.size() == 0) {  //io wait is the final operation
                p->done = true;
                p->end = true;
            }
            else {
                p->io_wait = true;
                p->end = true;
                p->T.push_back(p->burst);
                p->burst = 0;
                double burst_estimate = simple_estimate(p->T);
                p->S.push_back(burst_estimate);
                p->burst_count++;
                io_wait_list.push_back(p);
            }
            break;
        }

        case 6:
            if(locked(locks, arg)){
                p->codes.push_front(arg);
                p->codes.push_front(op);
                p->lock_wait = true;
            }
            else {
                locks.push_back(arg);
                p->lock_wait = false;
            }
            break;
        case 7:
            for(int i=0; i<locks.size(); i++) {
                if(locks[i] == arg) {
                    iter += i;
                    locks.erase(iter);
                }
            }
            break;
        default:
            cout<<"wrong"<<endl;
    }
    if(p->codes.size() == 0) {
        p->done = true;
        p->end = true;
    }
    time_interval++;
}

void operate_sjfe(Process* p, int cycle, vector<int> &locks, vector<Process*> &processes, vector<Process*> &sleep_list, vector<Process*> &io_wait_list, int* p_memory, int PMSize, int &aid, vector<Allocation*> &all_allocs, int &page_fault, string page, int &time_interval) {
    vector<int>::iterator iter = locks.begin();
    int op = p->codes.front();
    p->codes.pop_front();
    int arg = p->codes.front();
    p->codes.pop_front();
    p->line++;
    p->burst++;
    p->S[p->burst_count] -= 1;
    p->o = op;
    p->a = arg;

    if(time_interval == 8) {
        for(int i=0; i<all_allocs.size(); i++) {
            if( all_allocs[i]->in_pm) {
                for(int j=6; j >= 0; j-- ) {
                    all_allocs[i]->r_byte[j+1] = all_allocs[i]->r_byte[j];
                }
                all_allocs[i]->r_byte[0] = all_allocs[i]->r_bit;
                all_allocs[i]->r_bit = 0;
            }
        }
        time_interval = 0;
    }

    switch (op) {
        case 0: {
            int idx = get_pt_spot(p->pt_aid, arg, p->pt_size);
            Allocation* a = new Allocation(arg, idx, aid);
            p->allocations.push_back(a);
            all_allocs.push_back(a);
            for(int i=0; i<arg; i++) {
                p->pt_aid[i+idx] = a->aid;
                p->pt_v[i+idx] = 0;
            }
            aid++;
            break;
        }
        case 1: {
            Allocation* a;
            for(int i=0; i<all_allocs.size(); i++) {
                if(all_allocs[i]->aid == arg) {
                    a = all_allocs[i];
                    break;
                }
            }
            a->accessed_count++;
            a->last_used = cycle;
            a->r_bit = 1;
            if(!a->in_pm) {
                page_fault++;
                int idx = get_inPM_idx(a, p_memory, PMSize);
                if(idx != -1) {
                    a->pm_start = idx;
                    allocate(a, p_memory, p->pt_v, cycle);
                }
                else {
                    if(page == "fifo")
                        a->pm_start = fifo(p_memory, PMSize, a, all_allocs, processes);
                    else if(page == "lru")
                        a->pm_start = lru(p_memory, PMSize, a, all_allocs, processes);
                    else if(page == "lru-sampled")
                        a->pm_start = s_lru(p_memory, PMSize, a, all_allocs, processes);
                    else if(page == "lfu")
                        a->pm_start = lfu(p_memory, PMSize, a, all_allocs, processes);
                    else if(page == "mfu")
                        a->pm_start = mfu(p_memory, PMSize, a, all_allocs, processes);
                    else
                        a->pm_start = optimal(p_memory, PMSize, a, all_allocs, processes);
                    allocate(a, p_memory, p->pt_v, cycle);
                }
            }
            break;
        }
        case 2: {
            for(int i=0; i<p->pt_size; i++) {
                if(p->pt_aid[i] == arg) {
                    p->pt_aid[i] = -1;
                    p->pt_v[i] = -1;
                }
            }
            for(int i=0; i<p->allocations.size(); i++) {
                Allocation* a =p->allocations[i] ;
                if(a->aid == arg) {
                    a->alloced = false;
                    a->in_pm = false;
                    a->r_bit = 0;
                    for(int j=0; j<8; j++) {
                        a->r_byte[j] = 0;
                    }
                    a->byte_value = 0;
                    for(int j=0; j<PMSize; j++) {
                        if(p_memory[j] == a->aid)
                            p_memory[j] = -1;
                    }
                    break;
                }
            }
            vector<Allocation*>::iterator itr;
            itr = all_allocs.begin();
            for(int i=0; i<all_allocs.size(); i++) {
                if(all_allocs[i]->aid == arg) {
                    itr += i;
                    all_allocs.erase(itr);
                    break;
                }
            }
            break;
        }
        case 3:
            break;
        case 4:
            if(p->codes.size() == 0) {
                p->done = true;
                p->end = true;
            }
            else{
                p->sleep = cycle + arg;
                p->end = true;
                p->T.push_back(p->burst);
                p->burst = 0;
                double burst_estimate = exponential_estimate(p->T);
                p->S.push_back(burst_estimate);
                p->burst_count++;
                sleep_list.push_back(p);

            }
            break;
        case 5: {
            if(p->codes.size() == 0) {
                p->done = true;
                p->end = true;
            }
            else {
                p->io_wait = true;
                p->end = true;
                p->T.push_back(p->burst);
                p->burst = 0;
                double burst_estimate = exponential_estimate(p->T);
                p->S.push_back(burst_estimate);
                p->burst_count++;
                io_wait_list.push_back(p);
            }
            break;
        }

        case 6:
            if(locked(locks, arg)){
                p->codes.push_front(arg);
                p->codes.push_front(op);
                p->lock_wait = true;
            }
            else {
                locks.push_back(arg);
                p->lock_wait = false;
            }
            break;
        case 7:
            for(int i=0; i<locks.size(); i++) {
                if(locks[i] == arg) {
                    iter += i;
                    locks.erase(iter);
                }
            }
            break;
        default:
            cout<<"wrong"<<endl;
    }
    if(p->codes.size() == 0) {
        p->done = true;
        p->end = true;
    }
    time_interval++;
}

//get process with the least burst
Process* get_next(deque<Process*> &run_queue) {
    Process* next_p;
    int size = run_queue[0]->S.size();
    double remaining_burst = run_queue[0]->S[size-1];
    for(int i=0; i<run_queue.size(); i++) {
        size = run_queue[i]->S.size();
        double compare = run_queue[i]->S[size-1];
        if(remaining_burst > compare)
            remaining_burst = compare;
    }
    deque<Process*>::iterator run_itr;
    run_itr = run_queue.begin();
    for(int i=0; i<run_queue.size(); i++) {
        int idx = run_queue[i]->S.size()-1;
        if(run_queue[i]->S[idx] == remaining_burst) {
            next_p = run_queue[i];
            run_itr += i;
            break;
        }
    }
    run_queue.erase(run_itr);
    return next_p;
}

//return the least burst values among total processes
double least_burst(deque<Process*> &run_queue) {
    int size = run_queue[0]->S.size();
    double least_burst = run_queue[0]->S[size-1];
    for(int i=0; i<run_queue.size(); i++) {
        size = run_queue[i]->S.size();
        double compare = run_queue[i]->S[size-1];
        if(least_burst > compare)
            least_burst = compare;
    }
    return least_burst;
}

void fcfs(vector<Process*> &processes, vector<Operation*> operations, int total_event, int* p_memory, int PMSize, int VMSize, string &page, string dir) {
    int pid_count = 0;
    int cycle = 0;
    int aid = 1;    //initial aid
    int next_op =0; //next operation in input file
    int page_fault = 0;
    int time_interval = 0;
    bool more = true;
    bool p_done = false;
    Process* next_p = nullptr;
    Process* sched_p;
    deque<Process*> run_queue;
    vector<int> locks;
    vector<Process*> sleep_list;
    vector<Process*>::iterator sleep_itr;
    vector<Process*> io_wait_list;
    vector<Process*>::iterator iowait_itr;
    vector<Allocation*> all_allocs;

    FILE* out;
    string out_name = dir + "/scheduler.txt";
    out = fopen(out_name.c_str(), "w");

    FILE* out_p;
    string out_p_name = dir + "/memory.txt";
    out_p = fopen(out_p_name.c_str(), "w");


    while(more) {
        cycle++;
        sched_p = nullptr;  //reset scheduled process

        if(next_p != nullptr && next_p->lock_wait){ //busy waiting
            next_p->line--;     //restore line and time interval value
            time_interval--;
        }

        if(next_p != nullptr && next_p->done) { //if process finished
            for(int i=0; i<next_p->allocations.size(); i++) {   //release memory
                Allocation* a = next_p->allocations[i];
                for(int i=0; i<PMSize; i++) {
                    if(p_memory[i] == a->aid)
                        p_memory[i] = -1;
                }
                a->r_bit = 0;
                for(int i=0; i<8; i++) {
                    a->r_byte[i] = 0;
                }
                //reset variables
                a->in_pm = false;
                a->pm_start = -1;
                a->accessed_count = 0;
                a->byte_value = 0;

            }
        }

        for(int i=0; i<processes.size(); i++) {
            if(processes[i]->sleep == cycle) {  //if sleep finished
                for(int j=0; j<sleep_list.size(); j++) {
                    if(sleep_list[j]->pid == processes[i]->pid) {
                        sleep_itr = sleep_list.begin();
                        sleep_itr += j;
                        sleep_list.erase(sleep_itr);    //erase from sleep list
                    }
                }
                run_queue.push_back(processes[i]);  //push to run queue
            }
        }
        if(next_op<total_event) {
            for (int i = 0; i < operations.size(); i++) {
                if (operations[i]->start_cycle == cycle) {  //all operations for this cycle
                    if (operations[i]->op == "INPUT") {     //input operation
                        Process *input_p = get_process_from_pid(processes, operations[i]->arg);     //get process
                        if (input_p != nullptr) {
                            input_p->io_wait = false;
                            for (int j = 0; j < io_wait_list.size(); j++) {
                                if (io_wait_list[j]->pid == input_p->pid) {
                                    iowait_itr = io_wait_list.begin();
                                    iowait_itr += j;
                                    io_wait_list.erase(iowait_itr);     //release from io wait list
                                }
                            }
                            if (input_p->codes.size() == 0) {
                                input_p->done = true;
                            }
                            else {
                                run_queue.push_back(input_p);
                            }
                        }
                    }
                }
            }
        }

        if(next_op<total_event) {
            for(int i=0; i<operations.size(); i++) {
                if(operations[i]->start_cycle == cycle) {   //all operations for this cycle
                    if(operations[i]->op != "INPUT") {  //new process
                        Process* p = new Process(dir, operations[i]->op, pid_count, VMSize);
                        pid_count++;
                        processes.push_back(p);
                        run_queue.push_back(p);
                    }
                }
            }
        }
        int ops = 0;
        for(int i=0; i<operations.size(); i++) {
            if(operations[i]->start_cycle == cycle)
                ops++;
        }
        next_op += ops;     //increase number of next operation


        if(next_p == nullptr) {
            if(!run_queue.empty()){
                next_p = run_queue.front();
                run_queue.pop_front();
                sched_p = next_p;
            }
        }

        else if(next_p->end) {
            next_p->end = false;

            if(!run_queue.empty()){
                next_p = run_queue.front();
                run_queue.pop_front();
                sched_p = next_p;
            }
            else {
                next_p = nullptr;
            }
        }
        if(next_p != nullptr) {
            operate(next_p, cycle, locks, processes, sleep_list, io_wait_list, p_memory, PMSize, aid, all_allocs, page_fault, page, time_interval);
        }



        if(all_Finished(processes))
            more = false;

        fprintf(out, "[%d Cycle] Scheduled Process: ", cycle);
        if(sched_p == nullptr)
            fprintf(out, "None\n");
        else
            fprintf(out, "%d %s\n", sched_p->pid, sched_p->name.c_str());

        fprintf(out, "Running Process: ");
        if(next_p == nullptr)
            fprintf(out, "None\n");
        else
            fprintf(out, "Process#%d running code %s line %d(op %d, arg %d)\n", next_p->pid, next_p->name.c_str(), next_p->line, next_p->o, next_p->a);

        fprintf(out, "RunQueue: ");
        if(run_queue.empty())
            fprintf(out, "Empty");
        else {
            for(int i=0; i<run_queue.size(); i++) {
                string name = run_queue[i]->name;
                int pid = run_queue[i]->pid;
                fprintf(out, "%d(%s) ", pid, name.c_str());
            }
        }
        fprintf(out, "\n");

        fprintf(out, "SleepList: ");
        if(sleep_list.empty())
            fprintf(out, "Empty");
        else {
            for(int i=0; i<sleep_list.size(); i++) {
                string name = sleep_list[i]->name;
                int pid = sleep_list[i]->pid;
                fprintf(out, "%d(%s) ", pid, name.c_str());
            }
        }
        fprintf(out, "\n");

        fprintf(out, "IOWait List: ");
        if(io_wait_list.empty())
            fprintf(out, "Empty");
        else {
            for(int i=0; i<io_wait_list.size(); i++) {
                string name = io_wait_list[i]->name;
                int pid = io_wait_list[i]->pid;
                fprintf(out, "%d(%s) ", pid, name.c_str());
            }
        }
        fprintf(out, "\n");

        fprintf(out, "\n");

        fprintf(out_p, "[%d Cycle] Input : ", cycle);
        if(next_p != nullptr) {
            switch (next_p->o) {
                case 0: {
                    fprintf(out_p, "PID [%d] Function [ALLOCATION] Alloc ID [%d] Page Num[%d]\n",next_p->pid, aid-1, next_p->a);
                    break;
                }
                case 1: {
                    Allocation* a;
                    for(int i=0; i<next_p->allocations.size(); i++) {
                        if(next_p->allocations[i]->aid == next_p->a) {
                            a = next_p->allocations[i];
                            break;
                        }
                    }
                    fprintf(out_p, "PID [%d] Function [ACCESS] Alloc ID [%d] Page Num[%d]\n", next_p->pid, next_p->a, a->size);
                    break;
                }
                case 2: {
                    Allocation* a;
                    for(int i=0; i<next_p->allocations.size(); i++) {
                        if(next_p->allocations[i]->aid == next_p->a) {
                            a = next_p->allocations[i];
                            break;
                        }
                    }
                    fprintf(out_p, "PID [%d] Function [RELEASE] Alloc ID [%d] Page Num[%d]\n", next_p->pid, next_p->a, a->size);
                    break;
                }
                case 3: {
                    fprintf(out_p, "PID [%d] Function [NONMEMORY]\n", next_p->pid);
                    break;
                }
                case 4: {
                    fprintf(out_p, "PID [%d] Function [SLEEP]\n", next_p->pid);
                    break;
                }
                case 5: {
                    fprintf(out_p, "PID [%d] Function [IOWAIT]\n", next_p->pid);
                    break;
                }
                case 6: {
                    fprintf(out_p, "PID [%d] Function [LOCK]\n", next_p->pid);
                    break;
                }
                case 7: {
                    fprintf(out_p, "PID [%d] Function [UNLOCK]\n", next_p->pid);
                    break;
                }
            }
        }
        else {
            fprintf(out_p, "Function [NO-OP]\n");
        }
        fprintf(out_p, "%-30s", ">> Physical Memory : ");
        for(int i=0; i< PMSize/4; i++) {
            fprintf(out_p,"|");
            for(int j=0; j<4; j++) {

                if(p_memory[j+(i*4)] != -1)
                    fprintf(out_p, "%d", p_memory[j+(i*4)]);
                else
                    fprintf(out_p, "-");

            }
        }
        fprintf(out_p, "|\n");

        for(int i=0; i<processes.size(); i++) {
            if(!processes[i]->done || (next_p != nullptr && processes[i]->pid == next_p->pid)) {
                fprintf(out_p, ">> pid(%d) %-20s", processes[i]->pid, "Page Table(AID) : ");
                for(int j=0; j< VMSize/4; j++) {
                    fprintf(out_p,"|");
                    for(int k=0; k<4; k++) {
                        if(processes[i]->pt_aid[k+(j*4)] != -1)
                            fprintf(out_p, "%d", processes[i]->pt_aid[k+(j*4)]);
                        else
                            fprintf(out_p, "-");
                    }
                }
                fprintf(out_p, "|\n");

                fprintf(out_p, ">> pid(%d) %-20s", processes[i]->pid, "Page Table(VALID) : ");
                for(int j=0; j< VMSize/4; j++) {
                    fprintf(out_p,"|");
                    for(int k=0; k<4; k++) {
                        if(processes[i]->pt_v[k+(j*4)] != -1)
                            fprintf(out_p, "%d", processes[i]->pt_v[k+(j*4)]);
                        else
                            fprintf(out_p, "-");
                    }
                }
                fprintf(out_p, "|\n");
            }
        }
        fprintf(out_p, "\n");

    }
    fprintf(out_p, "page fault = %d\n", page_fault);

}

void rr(vector<Process*> &processes, vector<Operation*> operations, int total_event, int* p_memory, int PMSize, int VMSize, string &page, string dir) {
    int time_quantum = 10;
    int current_quantum = 0;
    int pid_count = 0;
    int cycle = 0;
    int aid = 1;
    int next_op =0;
    int page_fault = 0;
    int time_interval = 0;
    bool more = true;
    bool p_done = false;
    Process* next_p = nullptr;
    Process* sched_p;
    deque<Process*> run_queue;
    vector<int> locks;
    vector<Process*> sleep_list;
    vector<Process*>::iterator sleep_itr;
    vector<Process*> io_wait_list;
    vector<Process*>::iterator iowait_itr;
    vector<Allocation*> all_allocs;

    FILE* out;
    string out_name = dir + "/scheduler.txt";
    out = fopen(out_name.c_str(), "w");

    FILE* out_p;
    string out_p_name = dir + "/memory.txt";
    out_p = fopen(out_p_name.c_str(), "w");

    while(more) {
        current_quantum++;
        cycle++;
        sched_p = nullptr;

        if(next_p != nullptr && next_p->lock_wait){
            next_p->line--;
            time_interval--;
        }

        if(next_p != nullptr && next_p->done) {
            for(int i=0; i<next_p->allocations.size(); i++) {
                Allocation* a = next_p->allocations[i];
                for(int i=0; i<PMSize; i++) {
                    if(p_memory[i] == a->aid)
                        p_memory[i] = -1;
                }
                a->r_bit = 0;
                for(int i=0; i<8; i++) {
                    a->r_byte[i] = 0;
                }
                a->in_pm = false;
                a->pm_start = -1;
                a->accessed_count = 0;
                a->byte_value = 0;

            }
        }

        if(next_p != nullptr) {
            if(current_quantum>10 && !next_p->end) {    //time quantum finished
                next_p = nullptr;
            }
        }


        for(int i=0; i<processes.size(); i++) {
            if(processes[i]->sleep == cycle) {
                for(int j=0; j<sleep_list.size(); j++) {
                    if(sleep_list[j]->pid == processes[i]->pid) {
                        sleep_itr = sleep_list.begin();
                        sleep_itr += j;
                        sleep_list.erase(sleep_itr);
                    }
                }
                run_queue.push_back(processes[i]);
            }
        }
        if(next_op<total_event) {
            for (int i = 0; i < operations.size(); i++) {
                if (operations[i]->start_cycle == cycle) {
                    if (operations[i]->op == "INPUT") {
                        Process *input_p = get_process_from_pid(processes, operations[i]->arg);
                        if (input_p != nullptr) {
                            input_p->io_wait = false;
                            for (int j = 0; j < io_wait_list.size(); j++) {
                                if (io_wait_list[j]->pid == input_p->pid) {
                                    iowait_itr = io_wait_list.begin();
                                    iowait_itr += j;
                                    io_wait_list.erase(iowait_itr);
                                }
                            }
                            if (input_p->codes.size() == 0) {
                                input_p->done = true;
                            }
                            else {
                                run_queue.push_back(input_p);
                            }
                        }
                    }
                }
            }
        }

        if(next_op<total_event) {
            for(int i=0; i<operations.size(); i++) {
                if(operations[i]->start_cycle == cycle) {
                    if(operations[i]->op != "INPUT") {
                        Process* p = new Process(dir, operations[i]->op, pid_count, VMSize);
                        pid_count++;
                        processes.push_back(p);
                        run_queue.push_back(p);
                    }
                }
            }
        }
        int ops = 0;
        for(int i=0; i<operations.size(); i++) {
            if(operations[i]->start_cycle == cycle)
                ops++;
        }
        next_op += ops;


        if(next_p == nullptr) {
            if(!run_queue.empty()){
                next_p = run_queue.front();
                run_queue.pop_front();
                sched_p = next_p;
                current_quantum = 1;

            }
        }

        else if(next_p->end) {
            next_p->end = false;

            if(!run_queue.empty()){
                next_p = run_queue.front();
                run_queue.pop_front();
                sched_p = next_p;
                current_quantum = 1;
            }
            else {
                next_p = nullptr;
            }
        }
        if(next_p != nullptr) {
            operate(next_p, cycle, locks, processes, sleep_list, io_wait_list, p_memory, PMSize, aid, all_allocs, page_fault, page, time_interval);
        }
        if(next_p != nullptr) {
            if(current_quantum>9 && !next_p->end) {     //time quantum finished
                run_queue.push_back(next_p);    //push to run queue
            }
        }

        if(all_Finished(processes))
            more = false;

        fprintf(out, "[%d Cycle] Scheduled Process: ", cycle);
        if(sched_p == nullptr)
            fprintf(out, "None\n");
        else
            fprintf(out, "%d %s\n", sched_p->pid, sched_p->name.c_str());

        fprintf(out, "Running Process: ");
        if(next_p == nullptr)
            fprintf(out, "None\n");
        else
            fprintf(out, "Process#%d running code %s line %d(op %d, arg %d)\n", next_p->pid, next_p->name.c_str(), next_p->line, next_p->o, next_p->a);

        fprintf(out, "RunQueue: ");
        if(run_queue.empty())
            fprintf(out, "Empty");
        else {
            for(int i=0; i<run_queue.size(); i++) {
                string name = run_queue[i]->name;
                int pid = run_queue[i]->pid;
                fprintf(out, "%d(%s) ", pid, name.c_str());
            }
        }
        fprintf(out, "\n");

        fprintf(out, "SleepList: ");
        if(sleep_list.empty())
            fprintf(out, "Empty");
        else {
            for(int i=0; i<sleep_list.size(); i++) {
                string name = sleep_list[i]->name;
                int pid = sleep_list[i]->pid;
                fprintf(out, "%d(%s) ", pid, name.c_str());
            }
        }
        fprintf(out, "\n");

        fprintf(out, "IOWait List: ");
        if(io_wait_list.empty())
            fprintf(out, "Empty");
        else {
            for(int i=0; i<io_wait_list.size(); i++) {
                string name = io_wait_list[i]->name;
                int pid = io_wait_list[i]->pid;
                fprintf(out, "%d(%s) ", pid, name.c_str());
            }
        }
        fprintf(out, "\n");

        fprintf(out, "\n");

        fprintf(out_p, "[%d Cycle] Input : ", cycle);
        if(next_p != nullptr) {
            switch (next_p->o) {
                case 0: {
                    fprintf(out_p, "PID [%d] Function [ALLOCATION] Alloc ID [%d] Page Num[%d]\n",next_p->pid, aid-1, next_p->a);
                    break;
                }
                case 1: {
                    Allocation* a;
                    for(int i=0; i<next_p->allocations.size(); i++) {
                        if(next_p->allocations[i]->aid == next_p->a) {
                            a = next_p->allocations[i];
                            break;
                        }
                    }
                    fprintf(out_p, "PID [%d] Function [ACCESS] Alloc ID [%d] Page Num[%d]\n", next_p->pid, next_p->a, a->size);
                    break;
                }
                case 2: {
                    Allocation* a;
                    for(int i=0; i<next_p->allocations.size(); i++) {
                        if(next_p->allocations[i]->aid == next_p->a) {
                            a = next_p->allocations[i];
                            break;
                        }
                    }
                    fprintf(out_p, "PID [%d] Function [RELEASE] Alloc ID [%d] Page Num[%d]\n", next_p->pid, next_p->a, a->size);
                    break;
                }
                case 3: {
                    fprintf(out_p, "PID [%d] Function [NONMEMORY]\n", next_p->pid);
                    break;
                }
                case 4: {
                    fprintf(out_p, "PID [%d] Function [SLEEP]\n", next_p->pid);
                    break;
                }
                case 5: {
                    fprintf(out_p, "PID [%d] Function [IOWAIT]\n", next_p->pid);
                    break;
                }
                case 6: {
                    fprintf(out_p, "PID [%d] Function [LOCK]\n", next_p->pid);
                    break;
                }
                case 7: {
                    fprintf(out_p, "PID [%d] Function [UNLOCK]\n", next_p->pid);
                    break;
                }
            }
        }
        else {
            fprintf(out_p, "Function [NO-OP]\n");
        }
        fprintf(out_p, "%-30s", ">> Physical Memory : ");
        for(int i=0; i< PMSize/4; i++) {
            fprintf(out_p,"|");
            for(int j=0; j<4; j++) {

                if(p_memory[j+(i*4)] != -1)
                    fprintf(out_p, "%d", p_memory[j+(i*4)]);
                else
                    fprintf(out_p, "-");

            }
        }
        fprintf(out_p, "|\n");

        for(int i=0; i<processes.size(); i++) {
            if(!processes[i]->done || (next_p != nullptr && processes[i]->pid == next_p->pid)) {
                fprintf(out_p, ">> pid(%d) %-20s", processes[i]->pid, "Page Table(AID) : ");
                for(int j=0; j< VMSize/4; j++) {
                    fprintf(out_p,"|");
                    for(int k=0; k<4; k++) {
                        if(processes[i]->pt_aid[k+(j*4)] != -1)
                            fprintf(out_p, "%d", processes[i]->pt_aid[k+(j*4)]);
                        else
                            fprintf(out_p, "-");
                    }
                }
                fprintf(out_p, "|\n");

                fprintf(out_p, ">> pid(%d) %-20s", processes[i]->pid, "Page Table(VALID) : ");
                for(int j=0; j< VMSize/4; j++) {
                    fprintf(out_p,"|");
                    for(int k=0; k<4; k++) {
                        if(processes[i]->pt_v[k+(j*4)] != -1)
                            fprintf(out_p, "%d", processes[i]->pt_v[k+(j*4)]);
                        else
                            fprintf(out_p, "-");
                    }
                }
                fprintf(out_p, "|\n");
            }
        }
        fprintf(out_p, "\n");

    }
    fprintf(out_p, "page fault = %d\n", page_fault);

}

void sjfs(vector<Process*> &processes, vector<Operation*> operations, int total_event, int* p_memory, int PMSize, int VMSize, string &page, string dir) {
    int pid_count = 0;
    int cycle = 0;
    int aid = 1;
    int next_op =0;
    int page_fault = 0;
    int time_interval = 0;
    bool more = true;
    bool p_done = false;
    Process* next_p = nullptr;
    Process* sched_p;
    deque<Process*> run_queue;
    vector<int> locks;
    vector<Process*> sleep_list;
    vector<Process*>::iterator sleep_itr;
    vector<Process*> io_wait_list;
    vector<Process*>::iterator iowait_itr;
    vector<Allocation*> all_allocs;

    FILE* out;
    string out_name = dir + "/scheduler.txt";
    out = fopen(out_name.c_str(), "w");

    FILE* out_p;
    string out_p_name = dir + "/memory.txt";
    out_p = fopen(out_p_name.c_str(), "w");

    while(more) {
        cycle++;
        sched_p = nullptr;

        if(next_p != nullptr && next_p->lock_wait){
            next_p->line--;
            time_interval--;
        }

        if(next_p != nullptr && next_p->done) {
            for(int i=0; i<next_p->allocations.size(); i++) {
                Allocation* a = next_p->allocations[i];
                for(int i=0; i<PMSize; i++) {
                    if(p_memory[i] == a->aid)
                        p_memory[i] = -1;
                }
                a->r_bit = 0;
                for(int i=0; i<8; i++) {
                    a->r_byte[i] = 0;
                }
                a->in_pm = false;
                a->pm_start = -1;
                a->accessed_count = 0;
                a->byte_value = 0;

            }
        }

        for(int i=0; i<processes.size(); i++) {
            if(processes[i]->sleep == cycle) {
                for(int j=0; j<sleep_list.size(); j++) {
                    if(sleep_list[j]->pid == processes[i]->pid) {
                        sleep_itr = sleep_list.begin();
                        sleep_itr += j;
                        sleep_list.erase(sleep_itr);
                    }
                }
                run_queue.push_back(processes[i]);
            }
            if(next_p != nullptr && next_p->end != true){
                if(run_queue.size() != 0){
                    if(next_p->S[next_p->burst_count] > least_burst(run_queue)) {
                        run_queue.push_back(next_p);
                        next_p = get_next(run_queue);
                        sched_p = next_p;
                    }
                }

            }
        }

        if(next_op<total_event) {
            for (int i = 0; i < operations.size(); i++) {
                if (operations[i]->start_cycle == cycle) {
                    if (operations[i]->op == "INPUT") {
                        Process *input_p = get_process_from_pid(processes, operations[i]->arg);
                        if (input_p != nullptr) {
                            input_p->io_wait = false;
                            for (int j = 0; j < io_wait_list.size(); j++) {
                                if (io_wait_list[j]->pid == input_p->pid) {
                                    iowait_itr = io_wait_list.begin();
                                    iowait_itr += j;
                                    io_wait_list.erase(iowait_itr);
                                }
                            }
                            if (input_p->codes.size() == 0) {
                                input_p->done = true;
                            } else {
                                run_queue.push_back(input_p);
                                if(next_p != nullptr && next_p->end != true){
                                    if(run_queue.size() != 0){
                                        if(next_p->S[next_p->burst_count] > least_burst(run_queue)) {
                                            run_queue.push_back(next_p);
                                            next_p = get_next(run_queue);
                                            sched_p = next_p;
                                        }
                                    }

                                }
                            }
                        }
                    }
                }
            }
        }

        if(next_op<total_event) {
            for(int i=0; i<operations.size(); i++) {
                if(operations[i]->start_cycle == cycle) {
                    if(operations[i]->op != "INPUT") {
                        Process* p = new Process(dir, operations[i]->op, pid_count, VMSize);
                        pid_count++;
                        processes.push_back(p);
                        run_queue.push_back(p);

                        if(next_p != nullptr && next_p->end != true){
                            if(run_queue.size() != 0){
                                if(next_p->S[next_p->burst_count] > least_burst(run_queue)) {
                                    run_queue.push_back(next_p);
                                    next_p = get_next(run_queue);
                                    sched_p = next_p;
                                }
                            }

                        }
                    }
                }
            }
        }

        int ops = 0;
        for(int i=0; i<operations.size(); i++) {
            if(operations[i]->start_cycle == cycle)
                ops++;
        }
        next_op += ops;

        if(next_p == nullptr) {
            if(!run_queue.empty()){
                next_p = get_next(run_queue);
                sched_p = next_p;
            }
        }

        else if(next_p->end) {
            next_p->end = false;

            if(!run_queue.empty()){
                next_p = get_next(run_queue);
                sched_p = next_p;
            }
            else {
                next_p = nullptr;
            }
        }
        if(next_p != nullptr) {
            operate_sjfs(next_p, cycle, locks, processes, sleep_list, io_wait_list, p_memory, PMSize, aid, all_allocs, page_fault, page, time_interval);
        }



        if(all_Finished(processes))
            more = false;

        fprintf(out, "[%d Cycle] Scheduled Process: ", cycle);
        if(sched_p == nullptr)
            fprintf(out, "None\n");
        else
            fprintf(out, "%d %s\n", sched_p->pid, sched_p->name.c_str());

        fprintf(out, "Running Process: ");
        if(next_p == nullptr)
            fprintf(out, "None\n");
        else
            fprintf(out, "Process#%d running code %s line %d(op %d, arg %d)\n", next_p->pid, next_p->name.c_str(), next_p->line, next_p->o, next_p->a);

        fprintf(out, "RunQueue: ");
        if(run_queue.empty())
            fprintf(out, "Empty");
        else {
            for(int i=0; i<run_queue.size(); i++) {
                string name = run_queue[i]->name;
                int pid = run_queue[i]->pid;
                fprintf(out, "%d(%s) ", pid, name.c_str());
            }
        }
        fprintf(out, "\n");

        fprintf(out, "SleepList: ");
        if(sleep_list.empty())
            fprintf(out, "Empty");
        else {
            for(int i=0; i<sleep_list.size(); i++) {
                string name = sleep_list[i]->name;
                int pid = sleep_list[i]->pid;
                fprintf(out, "%d(%s) ", pid, name.c_str());
            }
        }
        fprintf(out, "\n");

        fprintf(out, "IOWait List: ");
        if(io_wait_list.empty())
            fprintf(out, "Empty");
        else {
            for(int i=0; i<io_wait_list.size(); i++) {
                string name = io_wait_list[i]->name;
                int pid = io_wait_list[i]->pid;
                fprintf(out, "%d(%s) ", pid, name.c_str());
            }
        }
        fprintf(out, "\n");

        fprintf(out, "\n");

        fprintf(out_p, "[%d Cycle] Input : ", cycle);
        if(next_p != nullptr) {
            switch (next_p->o) {
                case 0: {
                    fprintf(out_p, "PID [%d] Function [ALLOCATION] Alloc ID [%d] Page Num[%d]\n",next_p->pid, aid-1, next_p->a);
                    break;
                }
                case 1: {
                    Allocation* a;
                    for(int i=0; i<next_p->allocations.size(); i++) {
                        if(next_p->allocations[i]->aid == next_p->a) {
                            a = next_p->allocations[i];
                            break;
                        }
                    }
                    fprintf(out_p, "PID [%d] Function [ACCESS] Alloc ID [%d] Page Num[%d]\n", next_p->pid, next_p->a, a->size);
                    break;
                }
                case 2: {
                    Allocation* a;
                    for(int i=0; i<next_p->allocations.size(); i++) {
                        if(next_p->allocations[i]->aid == next_p->a) {
                            a = next_p->allocations[i];
                            break;
                        }
                    }
                    fprintf(out_p, "PID [%d] Function [RELEASE] Alloc ID [%d] Page Num[%d]\n", next_p->pid, next_p->a, a->size);
                    break;
                }
                case 3: {
                    fprintf(out_p, "PID [%d] Function [NONMEMORY]\n", next_p->pid);
                    break;
                }
                case 4: {
                    fprintf(out_p, "PID [%d] Function [SLEEP]\n", next_p->pid);
                    break;
                }
                case 5: {
                    fprintf(out_p, "PID [%d] Function [IOWAIT]\n", next_p->pid);
                    break;
                }
                case 6: {
                    fprintf(out_p, "PID [%d] Function [LOCK]\n", next_p->pid);
                    break;
                }
                case 7: {
                    fprintf(out_p, "PID [%d] Function [UNLOCK]\n", next_p->pid);
                    break;
                }
            }
        }
        else {
            fprintf(out_p, "Function [NO-OP]\n");
        }
        fprintf(out_p, "%-30s", ">> Physical Memory : ");
        for(int i=0; i< PMSize/4; i++) {
            fprintf(out_p,"|");
            for(int j=0; j<4; j++) {

                if(p_memory[j+(i*4)] != -1)
                    fprintf(out_p, "%d", p_memory[j+(i*4)]);
                else
                    fprintf(out_p, "-");

            }
        }
        fprintf(out_p, "|\n");

        for(int i=0; i<processes.size(); i++) {
            if(!processes[i]->done || (next_p != nullptr && processes[i]->pid == next_p->pid)) {
                fprintf(out_p, ">> pid(%d) %-20s", processes[i]->pid, "Page Table(AID) : ");
                for(int j=0; j< VMSize/4; j++) {
                    fprintf(out_p,"|");
                    for(int k=0; k<4; k++) {
                        if(processes[i]->pt_aid[k+(j*4)] != -1)
                            fprintf(out_p, "%d", processes[i]->pt_aid[k+(j*4)]);
                        else
                            fprintf(out_p, "-");
                    }
                }
                fprintf(out_p, "|\n");

                fprintf(out_p, ">> pid(%d) %-20s", processes[i]->pid, "Page Table(VALID) : ");
                for(int j=0; j< VMSize/4; j++) {
                    fprintf(out_p,"|");
                    for(int k=0; k<4; k++) {
                        if(processes[i]->pt_v[k+(j*4)] != -1)
                            fprintf(out_p, "%d", processes[i]->pt_v[k+(j*4)]);
                        else
                            fprintf(out_p, "-");
                    }
                }
                fprintf(out_p, "|\n");
            }
        }
        fprintf(out_p, "\n");
    }
    fprintf(out_p, "page fault = %d\n", page_fault);
}

void sjfe(vector<Process*> &processes, vector<Operation*> operations, int total_event, int* p_memory, int PMSize, int VMSize, string &page, string dir) {
    int pid_count = 0;
    int cycle = 0;
    int aid = 1;
    int next_op =0;
    int page_fault = 0;
    int time_interval = 0;
    bool more = true;
    bool p_done = false;
    Process* next_p = nullptr;
    Process* sched_p;
    deque<Process*> run_queue;
    vector<int> locks;
    vector<Process*> sleep_list;
    vector<Process*>::iterator sleep_itr;
    vector<Process*> io_wait_list;
    vector<Process*>::iterator iowait_itr;
    vector<Allocation*> all_allocs;

    FILE* out;
    string out_name = dir + "/scheduler.txt";
    out = fopen(out_name.c_str(), "w");

    FILE* out_p;
    string out_p_name = dir + "/memory.txt";
    out_p = fopen(out_p_name.c_str(), "w");

    while(more) {
        cycle++;
        sched_p = nullptr;

        if(next_p != nullptr && next_p->lock_wait){
            next_p->line--;
            time_interval--;
        }

        if(next_p != nullptr && next_p->done) {
            for(int i=0; i<next_p->allocations.size(); i++) {
                Allocation* a = next_p->allocations[i];
                for(int i=0; i<PMSize; i++) {
                    if(p_memory[i] == a->aid)
                        p_memory[i] = -1;
                }
                a->r_bit = 0;
                for(int i=0; i<8; i++) {
                    a->r_byte[i] = 0;
                }
                a->in_pm = false;
                a->pm_start = -1;
                a->accessed_count = 0;
                a->byte_value = 0;

            }
        }

        for(int i=0; i<processes.size(); i++) {
            if(processes[i]->sleep == cycle) {
                for(int j=0; j<sleep_list.size(); j++) {
                    if(sleep_list[j]->pid == processes[i]->pid) {
                        sleep_itr = sleep_list.begin();
                        sleep_itr += j;
                        sleep_list.erase(sleep_itr);
                    }
                }
                run_queue.push_back(processes[i]);
            }
            if(next_p != nullptr && next_p->end != true){
                if(run_queue.size() != 0){
                    if(next_p->S[next_p->burst_count] > least_burst(run_queue)) {
                        run_queue.push_back(next_p);
                        next_p = get_next(run_queue);
                        sched_p = next_p;
                    }
                }

            }
        }

        if(next_op<total_event) {
            for (int i = 0; i < operations.size(); i++) {
                if (operations[i]->start_cycle == cycle) {
                    if (operations[i]->op == "INPUT") {
                        Process *input_p = get_process_from_pid(processes, operations[i]->arg);
                        if (input_p != nullptr) {
                            input_p->io_wait = false;
                            for (int j = 0; j < io_wait_list.size(); j++) {
                                if (io_wait_list[j]->pid == input_p->pid) {
                                    iowait_itr = io_wait_list.begin();
                                    iowait_itr += j;
                                    io_wait_list.erase(iowait_itr);
                                }
                            }
                            if (input_p->codes.size() == 0) {
                                input_p->done = true;
                            } else {
                                run_queue.push_back(input_p);
                                if(next_p != nullptr && next_p->end != true){
                                    if(run_queue.size() != 0){
                                        if(next_p->S[next_p->burst_count] > least_burst(run_queue)) {
                                            run_queue.push_back(next_p);
                                            next_p = get_next(run_queue);
                                            sched_p = next_p;
                                        }
                                    }

                                }
                            }
                        }
                    }
                }
            }
        }

        if(next_op<total_event) {
            for(int i=0; i<operations.size(); i++) {
                if(operations[i]->start_cycle == cycle) {
                    if(operations[i]->op != "INPUT") {
                        Process* p = new Process(dir, operations[i]->op, pid_count, VMSize);
                        pid_count++;
                        processes.push_back(p);
                        run_queue.push_back(p);

                        if(next_p != nullptr && next_p->end != true){
                            if(run_queue.size() != 0){
                                if(next_p->S[next_p->burst_count] > least_burst(run_queue)) {
                                    run_queue.push_back(next_p);
                                    next_p = get_next(run_queue);
                                    sched_p = next_p;
                                }
                            }

                        }
                    }
                }
            }
        }

        int ops = 0;
        for(int i=0; i<operations.size(); i++) {
            if(operations[i]->start_cycle == cycle)
                ops++;
        }
        next_op += ops;

        if(next_p == nullptr) {
            if(!run_queue.empty()){
                next_p = get_next(run_queue);
                sched_p = next_p;
            }
        }

        else if(next_p->end) {
            next_p->end = false;

            if(!run_queue.empty()){
                next_p = get_next(run_queue);
                sched_p = next_p;
            }
            else {
                next_p = nullptr;
            }
        }
        if(next_p != nullptr) {
            operate_sjfe(next_p, cycle, locks, processes, sleep_list, io_wait_list, p_memory, PMSize, aid, all_allocs, page_fault, page, time_interval);
        }



        if(all_Finished(processes))
            more = false;

        fprintf(out, "[%d Cycle] Scheduled Process: ", cycle);
        if(sched_p == nullptr)
            fprintf(out, "None\n");
        else
            fprintf(out, "%d %s\n", sched_p->pid, sched_p->name.c_str());

        fprintf(out, "Running Process: ");
        if(next_p == nullptr)
            fprintf(out, "None\n");
        else
            fprintf(out, "Process#%d running code %s line %d(op %d, arg %d)\n", next_p->pid, next_p->name.c_str(), next_p->line, next_p->o, next_p->a);

        fprintf(out, "RunQueue: ");
        if(run_queue.empty())
            fprintf(out, "Empty");
        else {
            for(int i=0; i<run_queue.size(); i++) {
                string name = run_queue[i]->name;
                int pid = run_queue[i]->pid;
                fprintf(out, "%d(%s) ", pid, name.c_str());
            }
        }
        fprintf(out, "\n");

        fprintf(out, "SleepList: ");
        if(sleep_list.empty())
            fprintf(out, "Empty");
        else {
            for(int i=0; i<sleep_list.size(); i++) {
                string name = sleep_list[i]->name;
                int pid = sleep_list[i]->pid;
                fprintf(out, "%d(%s) ", pid, name.c_str());
            }
        }
        fprintf(out, "\n");

        fprintf(out, "IOWait List: ");
        if(io_wait_list.empty())
            fprintf(out, "Empty");
        else {
            for(int i=0; i<io_wait_list.size(); i++) {
                string name = io_wait_list[i]->name;
                int pid = io_wait_list[i]->pid;
                fprintf(out, "%d(%s) ", pid, name.c_str());
            }
        }
        fprintf(out, "\n");

        fprintf(out, "\n");

        fprintf(out_p, "[%d Cycle] Input : ", cycle);
        if(next_p != nullptr) {
            switch (next_p->o) {
                case 0: {
                    fprintf(out_p, "PID [%d] Function [ALLOCATION] Alloc ID [%d] Page Num[%d]\n",next_p->pid, aid-1, next_p->a);
                    break;
                }
                case 1: {
                    Allocation* a;
                    for(int i=0; i<next_p->allocations.size(); i++) {
                        if(next_p->allocations[i]->aid == next_p->a) {
                            a = next_p->allocations[i];
                            break;
                        }
                    }
                    fprintf(out_p, "PID [%d] Function [ACCESS] Alloc ID [%d] Page Num[%d]\n", next_p->pid, next_p->a, a->size);
                    break;
                }
                case 2: {
                    Allocation* a;
                    for(int i=0; i<next_p->allocations.size(); i++) {
                        if(next_p->allocations[i]->aid == next_p->a) {
                            a = next_p->allocations[i];
                            break;
                        }
                    }
                    fprintf(out_p, "PID [%d] Function [RELEASE] Alloc ID [%d] Page Num[%d]\n", next_p->pid, next_p->a, a->size);
                    break;
                }
                case 3: {
                    fprintf(out_p, "PID [%d] Function [NONMEMORY]\n", next_p->pid);
                    break;
                }
                case 4: {
                    fprintf(out_p, "PID [%d] Function [SLEEP]\n", next_p->pid);
                    break;
                }
                case 5: {
                    fprintf(out_p, "PID [%d] Function [IOWAIT]\n", next_p->pid);
                    break;
                }
                case 6: {
                    fprintf(out_p, "PID [%d] Function [LOCK]\n", next_p->pid);
                    break;
                }
                case 7: {
                    fprintf(out_p, "PID [%d] Function [UNLOCK]\n", next_p->pid);
                    break;
                }
            }
        }
        else {
            fprintf(out_p, "Function [NO-OP]\n");
        }
        fprintf(out_p, "%-30s", ">> Physical Memory : ");
        for(int i=0; i< PMSize/4; i++) {
            fprintf(out_p,"|");
            for(int j=0; j<4; j++) {

                if(p_memory[j+(i*4)] != -1)
                    fprintf(out_p, "%d", p_memory[j+(i*4)]);
                else
                    fprintf(out_p, "-");

            }
        }
        fprintf(out_p, "|\n");

        for(int i=0; i<processes.size(); i++) {
            if(!processes[i]->done || (next_p != nullptr && processes[i]->pid == next_p->pid)) {
                fprintf(out_p, ">> pid(%d) %-20s", processes[i]->pid, "Page Table(AID) : ");
                for(int j=0; j< VMSize/4; j++) {
                    fprintf(out_p,"|");
                    for(int k=0; k<4; k++) {
                        if(processes[i]->pt_aid[k+(j*4)] != -1)
                            fprintf(out_p, "%d", processes[i]->pt_aid[k+(j*4)]);
                        else
                            fprintf(out_p, "-");
                    }
                }
                fprintf(out_p, "|\n");

                fprintf(out_p, ">> pid(%d) %-20s", processes[i]->pid, "Page Table(VALID) : ");
                for(int j=0; j< VMSize/4; j++) {
                    fprintf(out_p,"|");
                    for(int k=0; k<4; k++) {
                        if(processes[i]->pt_v[k+(j*4)] != -1)
                            fprintf(out_p, "%d", processes[i]->pt_v[k+(j*4)]);
                        else
                            fprintf(out_p, "-");
                    }
                }
                fprintf(out_p, "|\n");
            }
        }
        fprintf(out_p, "\n");
    }
    fprintf(out_p, "page fault = %d\n", page_fault);

}

#endif //HW3_P_SCHEDULER_H
