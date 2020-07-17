#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <sstream>
#include <cmath>
#include <stdio.h>
#include "inode.h"
#include "dentry.h"

using namespace std;


void remove_file(Dentry* current_dentry, vector<Inode*> &inode_table, string name, int &used_blk) {
    vector<Inode*>::iterator inode_itr; //inode iterator
    vector<Inode*>::iterator inode_table_itr;   //inode table iterator
    inode_itr = current_dentry->d_inode.begin();
    inode_table_itr = inode_table.begin();

    int idx = -1;   //init inode idx
    int blocks = -1;    //init used blocks
    for(int i=0; i<current_dentry->d_inode.size(); i++) {   //find inode in current directory
        if(name == current_dentry->d_inode[i]->name){
            idx = i;
            blocks = current_dentry->d_inode[i]->total_blocks();    //get num of used disk blocks
            break;
        }
    }
    inode_itr += idx;
    current_dentry->d_inode.erase(inode_itr);   //remove from current directory

    for(int i=0; i<inode_table.size(); i++) {   //find inode in inode table
        if(name == inode_table[i]->name){
            idx = i;
            break;
        }
    }
    inode_table_itr += idx;
    inode_table.erase(inode_table_itr); //remove from inode table

    used_blk -= blocks; //reduce used blocks
}

void remove_directory(Dentry* current_dentry, vector<Inode*> &inode_table, string name, int &used_blk) {
    Dentry* p;  //directory pointer
    vector<Dentry*>::iterator dentry_itr;   //directory iterator
    dentry_itr = current_dentry->d_dentry.begin();
    int idx = -1;
    for(int i=0; i<current_dentry->d_dentry.size(); i++) {  //find directory in current directory
        if(name == current_dentry->d_dentry[i]->name) {
            p = current_dentry->d_dentry[i];
            idx = i;
            break;
        }
    }
    dentry_itr += idx;
    current_dentry->d_dentry.erase(dentry_itr); //remove directory from current directory

    //remove all files
    int inode_size = p->d_inode.size();
    vector<string> inode_names;
    for(int i=0; i<inode_size; i++) {
        inode_names.push_back(p->d_inode[i]->name);
    }
    for(int i=0; i<inode_size; i++) {
        string file_name = inode_names[i];
        remove_file(p, inode_table, file_name, used_blk);
    }

    //remove all subdirectories
    int dentry_size = p->d_dentry.size();
    vector<string> dentry_names;
    for(int i=0; i<dentry_size; i++) {
        dentry_names.push_back(p->d_dentry[i]->name);
    }
    for(int i=0; i<dentry_size; i++) {
        string dir_name = dentry_names[i];
        remove_directory(p, inode_table, dir_name, used_blk);
    }
}

void print_inodes(Dentry* current_dentry) {     //print all filenames in current directory
    priority_queue<int, vector<int>, greater<int> > id;  //priority queue for inode id
    for(int i=0; i<current_dentry->d_inode.size(); i++) {
        id.push(current_dentry->d_inode[i]->id);    //add inode id
    }
    while(!id.empty()) {
        int next = id.top();    //get smallest id
        id.pop();
        Inode* p;
        for(int i=0; i<current_dentry->d_inode.size(); i++) {
            if(next == current_dentry->d_inode[i]->id) {    //get inode with smallest id
                p = current_dentry->d_inode[i];
                break;
            }
        }
        printf("%s ", p->name.c_str()); //print file name
    }
}

bool check_id(vector<Inode*> inode_table, int inode_id) {   //check if inode id exists in inode table
    bool found = false;
    for(int i=0; i<inode_table.size(); i++) {
        if(inode_id == inode_table[i]->id) {    //inode found
            found = true;
            break;
        }
    }
    return found;
}

int next_id(vector<Inode*> inode_table) {   //return next available inode id
    int next = 0;
    bool found = false;

    while(!found) {
        if(check_id(inode_table, next))
            next++;
        else {
            found = true;   //no inode with current id
        }
    }
    return next;
}

int required_blocks(int file_size) {    //return total required blocks from file size
    int file_blocks = int(ceil(file_size / 1024.0));
    int required_blks;
    if(file_blocks <= 12) {     //only use direct blocks
        required_blks = file_blocks;
    }
    else if(file_blocks <= 268) {   //use up to single indirect blocks
        required_blks = file_blocks + 1;
    }
    else {  //use up to double indirect blocks
        int d_blks = int(ceil((file_blocks - 268) / 256.0));
        required_blks = file_blocks + 2 + d_blks;
    }
    return required_blks;
}

bool check_inode(Dentry* current_dentry, string name) { //check if inode exists in current directory
    bool found = false;
    for(int i=0; i<current_dentry->d_inode.size(); i++) {
        if(name == current_dentry->d_inode[i]->name) {  //inode found
            found = true;
            break;
        }
    }
    return found;
}

bool check_inode_in_table(vector<Inode*> table, string name) {
    bool found = false;
    for(int i=0; i<table.size(); i++) {
        if(name == table[i]->name) {
            found = true;
            break;
        }
    }
    return found;
}

bool check_dir(Dentry* current_dentry, string next) {   //check if directory exists in current directory
    if(next == ".") {   //if command is "."
        return true;
    }
    else if(next == "..") { //if command is ".."
        return current_dentry->parent != nullptr;
    }
    else {  //if command is directory name
        bool found = false;
        for(int i=0; i<current_dentry->d_dentry.size(); i++) {
            if(next == current_dentry->d_dentry[i]->name) { //found directory
                found = true;
                break;
            }
        }
        return found;
    }
}

bool dir_found(Dentry* current_dentry, vector<string> dir_names) {  //check if directory path exists
    bool found = false;
    Dentry* d = current_dentry;

    for(int i=0; i<dir_names.size(); i++) {
        string next = dir_names[i];
        if(check_dir(d, next)) {    //directory exists
            if(next == ".") {   //pass
            }
            else if(next == "..") {     //enter parent directory
                d = d->parent;
            }
            else {  //enter subdirectory
                for(int j=0; j<d->d_dentry.size(); j++) {
                    if(next == d->d_dentry[j]->name) {
                        d = d->d_dentry[j];
                        break;
                    }
                }
            }
        }
        else {  //directory not found
            return found;
        }
    }
    found = true;
    return found;
}

string get_current_path(Dentry* current_dentry) {   //return current directory path
    Dentry* p = current_dentry;
    string path = "";
    if(p->parent != nullptr) {  //not at root directory
        vector<string> paths;   //vector of directory names
        while(p->parent != nullptr) {
            paths.push_back(p->name);
            p = p->parent;
        }
        for(int i=paths.size()-1; i>=0; i--) {
            path += "/";
            path += paths[i];   //add directory names to string
        }
    }
    else {  //currently at root directory
        path += "/";
    }
    return path;
}

int main() {
    Dentry* root = new Dentry("", nullptr); //rood dentry
    Dentry* current_dentry = root;  //set current dentry as root;
    vector<Inode*> inode_table;     //all inodes
    int total_blk = 973;    //total disk blocks
    int used_blk = 0;       //currently used disk blocks

    bool exit = false;      //program exit flag

    while(!exit) {
        string cmd;
        printf("2015198005:%s$ ", get_current_path(current_dentry).c_str());
        getline(cin, cmd);  //get input command
        if(cmd != "") {
            char delim = ' ';       //split deliminator
            stringstream f(cmd);    //init stringstream
            vector<string> cmds;    //vector for command strings
            string s;
            while(getline(f, s, delim))     //split command into individual strings
                cmds.push_back(s);      //push string

            //exit program
            if(cmds[0] == "exit")
                exit = true;

            //directory info
            else if(cmds[0] == "ls") {
                for(int i=0; i<current_dentry->d_dentry.size(); i++) {
                    //print all subdirectories
                    printf("%s ", current_dentry->d_dentry[i]->name.c_str());
                }
                print_inodes(current_dentry);   //print all files
                printf("\n");
            }

            //change directory
            else if(cmds[0] == "cd") {
                Dentry* temp;
                string dir = cmds[1];

                if(dir.at(0) == '/') {
                    temp = root;
                    dir = dir.substr(1);
                }
                else {
                    temp = current_dentry;
                }
                stringstream d(dir);
                vector<string> dir_names;
                string dirs;
                char cut = '/';
                while(getline(d, dirs, cut))
                    dir_names.push_back(dirs);  //split directory command
                if(dir_found(temp, dir_names)) {
                    current_dentry = temp;
                    for(int i=0; i<dir_names.size(); i++) {
                        if(dir_names[i] == ".") {
                        }
                        else if(dir_names[i] == "..") {
                            current_dentry = current_dentry->parent;
                        }
                        else {
                            for(int j=0; j<current_dentry->d_dentry.size(); j++) {
                                if(dir_names[i] == current_dentry->d_dentry[j]->name) {
                                    current_dentry = current_dentry->d_dentry[j];
                                    break;
                                }
                            }
                        }
                    }
                }
                else {
                    printf("error\n");
                }
            }

            //make directory
            else if(cmds[0] == "mkdir") {
                bool error = false;     //error flag
                for(int i=1; i<cmds.size(); i++) {
                    if(!error) {
                        for(int j=0; j<current_dentry->d_dentry.size(); j++) {  //search current directory
                            string n = cmds[i];
                            if(n == current_dentry->d_dentry[j]->name) {    //directory name already exists
                                error = true;
                                break;
                            }
                        }
                    }
                }
                if(error) {
                    printf("error\n");
                }
                else {  //no error
                    for(int i=1; i<cmds.size(); i++) {
                        Dentry* d = new Dentry(cmds[i], current_dentry);    //make directory
                        current_dentry->d_dentry.push_back(d);  //add to current directory
                    }
                }
            }

            //remove directory
            else if(cmds[0] == "rmdir") {
                bool error = false;
                for(int i=1; i<cmds.size(); i++) {
                    if(!check_dir(current_dentry, cmds[i])) {   //directory does not exist
                        error = true;
                        break;
                    }
                }
                if(error) {
                    printf("error\n");
                }
                else {  //directory exists
                    for(int i=1; i<cmds.size(); i++) {
                        remove_directory(current_dentry, inode_table, cmds[i], used_blk);   //remove directory
                    }
                    //print remaining disk blocks
                    printf("Now you have ...\n");
                    printf("%d / 973 (blocks)\n", total_blk - used_blk);
                }
            }

            //make file
            else if(cmds[0] == "mkfile") {
                string file_name = cmds[1]; //get file name
                int file_size = stoi(cmds[2]);  //get file size
                int required_block = required_blocks(file_size);    //get required disk blocks
                //if not enough disk blocks or inode full
                if(required_block > total_blk - used_blk || inode_table.size() > 127 ) {
                    printf("error\n");
                }
                else {
                    if(check_inode(current_dentry, file_name)) {    //file exists
                        printf("error\n");
                    }
                    else {
                        int inode_id = next_id(inode_table);    //get inode id
                        Inode* i = new Inode(file_name, file_size, inode_id);   //make file
                        current_dentry->d_inode.push_back(i);   //push file into directory
                        inode_table.push_back(i);   //push file into inode table
                        used_blk += i->total_blocks();  //increase used disk blocks
                        //print remaining disk blocks
                        printf("Now you have ...\n");
                        printf("%d / 973 (blocks)\n", total_blk - used_blk);
                    }
                }
            }

            //remove file
            else if(cmds[0] == "rmfile") {
                bool error = false;
                for(int i=1; i<cmds.size(); i++) {
                    if(!check_inode(current_dentry, cmds[i])) { //if file does not exist
                        error = true;
                        break;
                    }
                }
                if(error) {
                    printf("error\n");
                }
                else {
                    for(int i=1; i<cmds.size(); i++) {
                        //remove files
                        remove_file(current_dentry, inode_table, cmds[i], used_blk);
                    }
                    //print remaining disk blocks
                    printf("Now you have ...\n");
                    printf("%d / 973 (blocks)\n", total_blk - used_blk);
                }
            }

            //inode
            else if(cmds[0] == "inode") {
                string file_name = cmds[1];
                if(check_inode(current_dentry, file_name)) {    //inode exist
                    Inode* p;   //inode pointer
                    for(int i=0; i<current_dentry->d_inode.size(); i++) {
                        if(file_name == current_dentry->d_inode[i]->name) {
                            p = current_dentry->d_inode[i];
                            break;
                        }
                    }
                    //print inode info
                    printf("ID: %d\nName: %s\nSize: %d (bytes)\n", p->id, p->name.c_str(), p->size);
                    printf("Direct blocks: %d\n", p->direct_block);
                    printf("Single indirect blocks: %d\n", p->single_block + p->single_indirect);
                    printf("Double indirect blocks: %d\n", p->double_block + p->double_indirect);
                }
                else {
                    printf("error\n");
                }
            }
        }

    }

    return 0;
}
