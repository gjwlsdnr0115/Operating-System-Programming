#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fstream>
#include <sys/time.h>
#include "data.h"

using namespace std;

void write_data(Data d, ofstream& file) {       //write filter, image data
    for(int i=0; i<d.dat_r; i++) {
        for(int j=0; j<d.dat_c; j++) {
            file<<d.chan1[i][j]<<" ";
        }
    }
    for(int i=0; i<d.dat_r; i++) {
        for(int j=0; j<d.dat_c; j++) {
            file<<d.chan2[i][j]<<" ";
        }
    }
    for(int i=0; i<d.dat_r; i++) {
        for(int j=0; j<d.dat_c; j++) {
            file<<d.chan3[i][j]<<" ";
        }
    }
}

int main(int argc, char* argv[]) {
    int total_process_num = atoi(argv[1]);      //total process number
    int run_process_num=0;          //created process number
    pid_t pids[total_process_num];  //process array
    int status;         // for wait function
    struct timeval start, end;      //start, end point of time
    long total_time_ms;             // total time

    int fil_n, fil_r, fil_c;
    scanf("%d" "%d" "%d", &fil_n, &fil_r, &fil_c);      //input filter number, row, column

    vector <Data*> fil_vec;     //filter vector
    for(int i=0; i<fil_n; i++) {
        Data* f = new Data(fil_r, fil_c);
        f->set_data(fil_r, fil_c);      //filter data input
        fil_vec.push_back(f);
    }

    int img_r, img_c;
    scanf("%d" "%d", &img_r, &img_c);       //input image row, column
    Data img = Data(img_r, img_c);
    img.set_data(img_r, img_c);     //image data input

    int div[total_process_num + 1];     //divide filters per process
    for(int i=0; i<total_process_num + 1; i++) {    //initialize array with 0
        div[i] = 0;
    }
    if(fil_n%total_process_num == 0) {      //if filter division is even
        for(int i=1; i<total_process_num + 1; i++) {
            div[i] = fil_n/total_process_num;
        }
    }
    else {      //if filter division is not even
        for(int i=1; i<fil_n%total_process_num + 1; i++) {   //filter + 1
            div[i] = fil_n/total_process_num + 1;
        }
        for(int i=fil_n%total_process_num + 1; i<total_process_num + 1; i++) {
            div[i] = fil_n/total_process_num;
        }
    }

    for(int i=1; i<total_process_num + 1; i++) {    //making temporary input file
        string num = to_string(i);
        string dir = "ptoc.txt";
        string name = num + dir;
        const char* file_name = name.c_str();       // temp file name
        ofstream file;          //output file stream
        file.open(file_name);   //open stream

        if(div[i] != 0){        //write file
            file<<div[i]<<" ";          //write filter number
            file<<fil_r<<" "<<fil_c<<" ";       //write filter row, column
            int current = 0;    //filter number initialize
            for(int j=0; j<i+1; j++) {      //current process filter num
                current += div[j];
            }
            int last = current - div[i];        //number of previous process's filter
            for(int j=last; j< current; j++) {     //write filters
                write_data(*fil_vec[j], file);
            }
            file<<img_r<<" "<<img_c<<" ";       //image row, column
            write_data(img, file);           //write image
        }
        else{       //process with no filters
            file<<0<<" "<<0<<" "<<0<<" "<<0<<" "<<0;
        }
        file.close();
    }

    gettimeofday(&start, NULL);     //start time

    while(run_process_num<total_process_num) {      //repeat until total process number
        pids[run_process_num] = fork();         //create process
        if(pids[run_process_num] < 0) {     //if fail
            perror("fork error");
            exit(EXIT_FAILURE);
        }
        else if(pids[run_process_num] == 0) {    //if child process
            int file_in, file_out;
            char *par[] = {"./program1", NULL};
            string num = to_string(run_process_num + 1);
            string dir_in = "ptoc.txt";
            string name_in = num + dir_in;
            string dir_out = "ctop.txt";
            string name_out = num + dir_out;
            const char* in_name = name_in.c_str();      //input file name
            const char* out_name = name_out.c_str();    //output file name

            file_in = open(in_name, O_RDONLY);
            file_out = open(out_name, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
            dup2(file_in, 0);       //redirect input
            dup2(file_out, 1);      //redirect output
            execvp("./program1", par);   //execute program1
            close(file_in);
            close(file_out);
            exit(EXIT_SUCCESS);
        }
        else {
        }
        run_process_num++;  //increase process num
    }

    for(int i=0; i<total_process_num; i++) {
        waitpid(pids[i], &status, WUNTRACED);       //wait for child process to finish
    }

    gettimeofday(&end, NULL);     //end time

    int res_r = img_r - (fil_r -3);    //result data row
    vector<int> times;  //total time vector
    for(int i=0; i<total_process_num; i++) {
        ifstream file_in;       //input file stream
        string num = to_string(i + 1);
        string dir_in = "ctop.txt";
        string name_in = num + dir_in;
        const char* in_name = name_in.c_str();
        file_in.open(in_name);  //open input stream

        if(div[i+1]==0) {   //if no filters in process
            int p_time;     //get process time
            file_in>>p_time;
            times.push_back(p_time);
        }
        else{
            for(int j=0; j<div[i+1]; j++) {
                string line;

                for(int k=0; k<(res_r+1); k++) {
                    getline(file_in, line);     //get result
                    cout<<line<<endl;
                }
            }
            int p_time;
            file_in>>p_time;
            times.push_back(p_time);
        }
        file_in.close();
    }

    for(int i=0; i<times.size(); i++) {     //print process's time
        printf("%d ", times[i]);
    }
    printf("\n");

    total_time_ms = 1000*((long)end.tv_sec - (long)start.tv_sec) + ((long)end.tv_usec - (long)start.tv_usec)/1000;    //total elapsed time
    printf("%ld\n", total_time_ms);

    for(int i=1; i<total_process_num +1; i++) {     //remove temp files
        string num = to_string(i);
        string dir_in = "ptoc.txt";
        string name_in = num + dir_in;
        string dir_out = "ctop.txt";
        string name_out = num + dir_out;
        const char* in_name = name_in.c_str();
        const char* out_name = name_out.c_str();
        remove(in_name);
        remove(out_name);
    }
    
    return 0;
}