#include <iostream>
#include <string>
#include <vector>
#include "process.h"
#include "operation.h"
#include "scheduler.h"
#include <list>
#include "allocation.h"
#include <queue>

using namespace std;

int main(int argc, char* argv[]) {
    //default values
    string sched = "fcfs";
    string page = "fifo";
    string dir = ".";

    //get arguments
    vector<string> args;
    for(int i=1; i<argc; i++) {
        args.push_back(argv[i]);
    }
    for(int i=0; i<args.size(); i++) {
        if(args[i].at(1) == 's')
            sched = args[i].substr(7, args[i].size()-7);
        else if(args[i].at(1) == 'p')
            page = args[i].substr(6, args[i].size()-6);
        else
            dir = args[i].substr(5, args[i].size()-5);
    }

    //initialize variables for input
    int total_event;
    int VMSize;
    int PMSize;
    int pageSize;

    //open input file
    ifstream file_in;
    file_in.open(dir + "/input");
    file_in>>total_event>>VMSize>>PMSize>>pageSize;

    //resize memory size
    int re_VMSize = VMSize / pageSize;
    int re_PMSize = PMSize / pageSize;

    //initialize physical memory
    int* p_memory = new int[re_PMSize];
    for(int i=0; i<re_PMSize; i++) {
        p_memory[i] =-1;
    }

    //init vector for processes and operations
    vector<Process*> processes;
    vector<Operation*> operations;

    //get operations from input file
    for(int i=0; i<total_event; i++) {
        int start_cycle;
        string op;
        int arg;
        file_in>>start_cycle>>op;
        if(op == "INPUT") {
            file_in>>arg;
            Operation* p = new Operation(start_cycle, op, arg);
            operations.push_back(p);
        }
        else {
            Operation* p = new Operation(start_cycle, op, -1);
            operations.push_back(p);
        }

    }

    //close file
    file_in.close();

    //select scheduler
    if(sched == "fcfs")
        fcfs(processes, operations, total_event, p_memory, re_PMSize, re_VMSize, page, dir);
    else if(sched == "rr")
        rr(processes, operations, total_event, p_memory, re_PMSize, re_VMSize, page, dir);
    else if(sched == "sjf-simple")
        sjfs(processes, operations, total_event, p_memory, re_PMSize, re_VMSize, page, dir);
    else    //sjf-exponential
        sjfe(processes, operations, total_event, p_memory, re_PMSize, re_VMSize, page, dir);



    return 0;
}
