//
// Created by 허진욱 on 2020/06/12.
//

#ifndef HW3_P_ALGORITHM_H
#define HW3_P_ALGORITHM_H

#include "allocation.h"
#include "process.h"
#include "math.h"

using namespace std;

void allocate(Allocation* a, int* p_memory, int* pt_v, int cycle) {
    //puts alloc object into PMemeory
    int pm_idx = a->pm_start;
    for(int i=0; i<a->alloc_size; i++) {
        p_memory[pm_idx + i] = a->aid;
    }
    int vm_idx = a->pt_start;
    for(int i=0; i<a->size; i++) {
        pt_v[vm_idx + i] = 1;
    }
    a->in_pm_cycle = cycle;
    a->in_pm = true;
}

//returns available index in PMemory
int get_inPM_idx(Allocation* a, int* p_memory, int PMSize) {
    int idx = -1;
    for(int i=0; i<PMSize; i+=a->alloc_size) {
        bool avail = true;
        for(int j=0; j<a->alloc_size; j++) {
            if(p_memory[j+i] != -1) {
                avail = false;
                break;
            }
        }
        if(avail) {
            idx = i;
            break;
        }
    }
    //return -1 if not available
    return idx;
}

//remove alloc object from PMemory
void take_out(int* p_memory, int PMSize, Allocation* a, vector<Process*> processes) {
    for(int i=0; i<PMSize; i++) {
        if(p_memory[i] == a->aid)
            p_memory[i] = -1;
    }
    a->in_pm = false;
    a->pm_start = -1;
    a->r_bit = 0;
    a->accessed_count = 0;
    a->byte_value = 0;
    for(int i=0; i<8; i++) {
        a->r_byte[i] = 0;
    }
    for(int i=0; i<processes.size(); i++) {
        Process* p = processes[i];
        for(int j=0; j<p->allocations.size(); j++) {
            if(p->allocations[j]->aid == a->aid) {
                int idx = -1;
                for(int k=0; k<p->pt_size; k++) {
                    if(p->pt_aid[k] == a->aid) {
                        idx = k;
                        break;
                    }
                }
                for(int k=0; k<a->size; k++) {
                    p->pt_v[k+idx] = 0;
                }
            }
        }
    }
}

int get_next_access(Process* p, int aid) {
    int idx = -1;

    for(int i=0; i<p->codes.size(); i+=2) {
        if(p->codes[i] == 1 && p->codes[i+1] == aid) {
            idx = i+1;
            break;
        }
    }
    return idx;
};

void update_next_acceess(vector<Process*> &processes) {
    for(int i=0; i<processes.size(); i++) {
        for(int j=0; j<processes[i]->allocations.size(); j++) {
            Allocation* a = processes[i]->allocations[j];
            a->next_access = get_next_access(processes[i], a->aid);
        }
    }
}

//return available index in PMemory
int fifo(int* p_memory, int PMSize, Allocation* in_pm, vector<Allocation*> &all_allocs, vector<Process*> &processes) {
    bool avail = false;
    int idx;
    while(!avail) { //no available spot
        int first_cycle;
        for(int i=0; i<all_allocs.size(); i++) {
            if(all_allocs[i]->in_pm) {
                first_cycle = all_allocs[i]->in_pm_cycle;
                break;
            }
        }
        //find least in PMemory cycle
        for(int i=0; i<all_allocs.size(); i++) {
            int in_cycle = all_allocs[i]->in_pm_cycle;
            if(first_cycle > in_cycle && all_allocs[i]->in_pm)
                first_cycle = in_cycle;
        }
        //get alloc obj
        Allocation *a;
        for(int i=0; i<all_allocs.size(); i++) {
            if(all_allocs[i]->in_pm_cycle == first_cycle) {
                a = all_allocs[i];
                break;
            }
        }
        //take out
        take_out(p_memory, PMSize, a, processes);
        idx = get_inPM_idx(in_pm, p_memory, PMSize);
        if(idx != -1)
            avail = true;

    }
    return idx;
}

int lru(int* p_memory, int PMSize, Allocation* in_pm, vector<Allocation*> &all_allocs, vector<Process*> &processes) {
    bool avail = false;
    int idx;
    while(!avail) {
        int oldest;
        for(int i=0; i<all_allocs.size(); i++) {
            if(all_allocs[i]->in_pm) {
                oldest = all_allocs[i]->last_used;
                break;
            }
        }
        //get least recently used cycle
        for(int i=0; i<all_allocs.size(); i++) {
            int in_cycle = all_allocs[i]->last_used;
            if(oldest > in_cycle && all_allocs[i]->in_pm)
                oldest = in_cycle;
        }
        //get alloc obj
        Allocation *a;
        for(int i=0; i<all_allocs.size(); i++) {
            if(all_allocs[i]->last_used == oldest) {
                a = all_allocs[i];
                break;
            }
        }
        //remove obj
        take_out(p_memory, PMSize, a, processes);
        idx = get_inPM_idx(in_pm, p_memory, PMSize);
        if(idx != -1)
            avail = true;

    }
    return idx;
}

int s_lru(int* p_memory, int PMSize, Allocation* in_pm, vector<Allocation*> &all_allocs, vector<Process*> &processes) {
    bool avail = false;
    int idx;
    while(!avail) {
        //get smallest reference bit/byte

        int least_size = 512;
        for(int i=0; i<all_allocs.size(); i++) {
            if(all_allocs[i]->in_pm) {
                int size = 0;
                for(int j=0; j<8; j++) {
                    size += all_allocs[i]->r_byte[j] * pow(2, (7-j));
                }
                size += pow(2, 8) * all_allocs[i]->r_bit;
                all_allocs[i]->byte_value = size;
                if(least_size > size)
                    least_size = size;
            }
        }

        //get smallest AID with least_size
        int aid = INT_MAX;
        for(int i=0; i<all_allocs.size(); i++) {
            if(all_allocs[i]->in_pm && all_allocs[i]->byte_value == least_size) {
                if(aid > all_allocs[i]->aid)
                    aid = all_allocs[i]->aid;
            }
        }
        //get alloc obj
        Allocation* a;
        for(int i=0; i<all_allocs.size(); i++) {
            if(all_allocs[i]->in_pm && all_allocs[i]->byte_value == least_size && all_allocs[i]->aid == aid) {
                a = all_allocs[i];
                break;
            }
        }
        //remove obj
        take_out(p_memory, PMSize, a, processes);
        idx = get_inPM_idx(in_pm, p_memory, PMSize);
        if(idx != -1)
            avail = true;

    }
    return idx;
}

int lfu(int* p_memory, int PMSize, Allocation* in_pm, vector<Allocation*> &all_allocs, vector<Process*> &processes) {
    bool avail = false;
    int idx;
    while(!avail) {
        int least_accessed;
        for(int i=0; i<all_allocs.size(); i++) {
            if(all_allocs[i]->in_pm) {
                least_accessed = all_allocs[i]->accessed_count;
                break;
            }
        }
        for(int i=0; i<all_allocs.size(); i++) {
            int accessed = all_allocs[i]->accessed_count;
            if(least_accessed > accessed && all_allocs[i]->in_pm) {
                least_accessed = accessed;
            }
        }
        //get smallest AID with least_accessed
        int aid = INT_MAX;
        for(int i=0; i<all_allocs.size(); i++) {
            if(all_allocs[i]->in_pm && all_allocs[i]->accessed_count == least_accessed) {
                if(aid > all_allocs[i]->aid)
                    aid = all_allocs[i]->aid;
            }
        }

        Allocation *a;
        for(int i=0; i<all_allocs.size(); i++) {
            if(all_allocs[i]->accessed_count == least_accessed && all_allocs[i]->in_pm && all_allocs[i]->aid == aid) {
                a = all_allocs[i];
                break;
            }
        }
        take_out(p_memory, PMSize, a, processes);
        idx = get_inPM_idx(in_pm, p_memory, PMSize);
        if(idx != -1)
            avail = true;
    }
    return idx;
}

int mfu(int* p_memory, int PMSize, Allocation* in_pm, vector<Allocation*> &all_allocs, vector<Process*> &processes) {
    bool avail = false;
    int idx;
    while(!avail) {
        int most_accessed = -1;

        for(int i=0; i<all_allocs.size(); i++) {
            int accessed = all_allocs[i]->accessed_count;
            if(most_accessed < accessed && all_allocs[i]->in_pm)
                most_accessed = accessed;
        }

        //get smallest AID with most_accessed
        int aid = INT_MAX;
        for(int i=0; i<all_allocs.size(); i++) {
            if(all_allocs[i]->in_pm && all_allocs[i]->accessed_count == most_accessed) {
                if(aid > all_allocs[i]->aid)
                    aid = all_allocs[i]->aid;
            }
        }

        Allocation *a;
        for(int i=0; i<all_allocs.size(); i++) {
            if(all_allocs[i]->accessed_count == most_accessed && all_allocs[i]->in_pm && all_allocs[i]->aid == aid) {
                a = all_allocs[i];
                break;
            }
        }
        take_out(p_memory, PMSize, a, processes);
        idx = get_inPM_idx(in_pm, p_memory, PMSize);
        if(idx != -1)
            avail = true;
    }
    return idx;
}

int optimal(int* p_memory, int PMSize, Allocation* in_pm, vector<Allocation*> &all_allocs, vector<Process*> &processes) {
    update_next_acceess(processes);
    bool avail = false;
    int idx;
    while(!avail) {
        int longest = -1;
        for(int i=0; i<all_allocs.size(); i++) {
            if(all_allocs[i]->in_pm && all_allocs[i]->next_access != -1) {
                int next = all_allocs[i]->next_access;
                if(longest < next)
                    longest = next;
            }
        }
        Allocation* a;
        for(int i=0; i<all_allocs.size(); i++) {
            if(all_allocs[i]->next_access == longest && all_allocs[i]->in_pm) {
                a = all_allocs[i];
                break;
            }
        }
        take_out(p_memory, PMSize, a, processes);
        idx = get_inPM_idx(in_pm, p_memory, PMSize);
        if(idx != -1)
            avail = true;
    }
    return idx;
}

#endif //HW3_P_ALGORITHM_H
