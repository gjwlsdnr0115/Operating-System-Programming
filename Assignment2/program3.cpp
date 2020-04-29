#include <iostream>
#include <string>
#include <vector>
#include <sys/time.h>
#include <pthread.h>
#include <mutex>
#include "data.h"

using namespace std;

int total_thread_num;   //total thread number
int filter_n, filter_r, filter_c;   //filter number, row, column
int image_r, image_c;           //image row, column
mutex m;    //mutex variable
vector <Data*> fil_vec;     //filter vector
Data* image;        //image data
int*** conv_results;    //convolution results
int result_r;           //result row
int result_c;           //result column
long* thread_times;     //thread time
int* divide;            //filter divide

int multiply(int** img, int** fil, int start_r, int start_c, int fil_r, int fil_c) {    //multiply image and filter
    int val = 0;        //initialize value of multiplication
    int r = start_r;    //row location of filter[0][0]
    int c = start_c;    //column location of filter[0][0]
    int fil_idx_r = 0;  //multiplying index of filter
    int fil_idx_c = 0;

    for(int i=0; i< fil_r; i++) {
        for(int j=0; j<fil_c; j++) {
            val += img[r][c] * fil[fil_idx_r][fil_idx_c];       //add result to val
            c++;            //increase image column
            fil_idx_c++;    //increase filter column
        }
        r++;                //increase image row
        fil_idx_r++;        //increase filter row
        c = start_c;        //reset image column to start
        fil_idx_c = 0;      //reset filter column to 0
    }
    return val;
}
void convolution(Data img, Data fil, int filter_num) {         //perform convolution
    int res_r = img.dat_r - (fil.dat_r - 1);    //result data row
    int res_c = img.dat_c - (fil.dat_c - 1);    //result data column
    Data result = Data(res_r, res_c);           //result data of convolution

    int*** results = new int**[3] {result.chan1, result.chan2, result.chan3};   //array containing result channels
    int*** images = new int**[3] {img.chan1, img.chan2, img.chan3};     //array containing image channels
    int*** filters = new int**[3] {fil.chan1, fil.chan2, fil.chan3};    //array containing filter channels

    for(int i=0; i<3; i++) {        // convolute image and filter
        for(int j=0; j<res_r; j++) {
            for(int k=0; k< res_c; k++)
                results[i][j][k] = multiply(images[i], filters[i], j, k, fil.dat_r, fil.dat_c);
        }
    }
    int** convoluted;       //final 2-array result
    convoluted = new int*[res_r];
    for(int i=0; i<res_r; i++) {
        convoluted[i] = new int[res_c];
        for(int j=0; j<res_c; j++) {        //add channels
            int val = results[0][i][j] + results[1][i][j] + results[2][i][j];
            if(val < 0) {convoluted[i][j] = 0;}     //perform ReLU function
            else {convoluted[i][j] = val;}
        }
    }
    m.lock();   //mutex lock
    conv_results[filter_num] = convoluted;
    m.unlock(); //mutex unlock
}
void *thread_conv(void *data) {     //thread function

    struct timeval start, end;      //time start, end
    long thread_time;       //elapsed thread time
    const int thread_num = (int)(size_t)(data);     //thread number
    int filter_num = 0;
    for(int i=0; i< thread_num+1; i++) {    //current thread filter number
        filter_num += divide[i];
    }
    int prev_num = filter_num - divide[thread_num];     //previous thread filtter number

    gettimeofday(&start, NULL);     //start time
    if(filter_num != 0)  {
        for(int i=prev_num; i<filter_num; i++) {
            convolution(*image, *fil_vec[i], i);
        }
    }
    gettimeofday(&end, NULL);       //end time
    thread_time = 1000*((long)end.tv_sec - (long)start.tv_sec) + ((long)end.tv_usec - (long)start.tv_usec)/1000;
    m.lock();       //mutex lock
    thread_times[thread_num] = thread_time;     //allocate time into array
    m.unlock();     //mutex unlock
    pthread_exit(NULL);     //exit thread
}

int main(int argc, char* argv[]) {
    int run_thread_num = 0;     //created thread number
    total_thread_num = atoi(argv[1]);       //total thread number
    long total_time_ms;         //total time
    struct timeval start, end;  //time start, end
    int tid;    //thread id
    int thread_status;  //thread status at exit
    pthread_t *threads = new pthread_t[total_thread_num];   //thread array

    scanf("%d" "%d" "%d", &filter_n, &filter_r, &filter_c); //input filter

    for(int i=0; i<filter_n; i++) {
        Data* f = new Data(filter_r, filter_c);
        f->set_data(filter_r, filter_c);
        fil_vec.push_back(f);
    }

    scanf("%d" "%d", &image_r, &image_c);   //input image
    image = new Data(image_r, image_c);
    image->set_data(image_r, image_c);
    image->pad_data();
    result_r = image->dat_r - (filter_r -1);
    result_c = image->dat_c - (filter_c -1);
    conv_results = new int**[filter_n];
    thread_times = new long[total_thread_num];

    divide = new int[total_thread_num];     //divide filter per thread
    if(filter_n%total_thread_num == 0) {
        for(int i=0; i<total_thread_num; i++) {
            divide[i] = filter_n/total_thread_num;
        }
    }
    else {
        for(int i=0; i<filter_n%total_thread_num; i++) {
            divide[i] = filter_n/total_thread_num+1;
        }
        for(int i=filter_n%total_thread_num; i<total_thread_num; i++) {
            divide[i] = filter_n/total_thread_num;
        }
    }
    long thread_nums[total_thread_num];
    for(int i=0; i<total_thread_num; i++) {
        thread_nums[i] = i;
    }

    gettimeofday(&start, NULL);     //start time
    while(run_thread_num<total_thread_num) {    //create thread
        tid = pthread_create(&threads[run_thread_num], NULL, thread_conv, (void *)thread_nums[run_thread_num]);
        if(tid != 0) {
            perror("create failed");
            exit(EXIT_FAILURE);
        }
        run_thread_num++;   //increase thread num
    }

    for(int i=0; i<total_thread_num; i++) {     //wait thread to exit
        tid = pthread_join(threads[i], (void **)&thread_status);
    }
    gettimeofday(&end, NULL);   //end time

    for(int i=0; i<filter_n; i++) {     //print result
        for(int j=0; j<result_r; j++) {
            for(int k=0; k<result_c; k++) {
                printf("%d ", conv_results[i][j][k]);
            }
            printf("\n");
        }
        printf("\n");
    }

    for(int i=0; i<total_thread_num; i++) { //print thread's time
        printf("%ld ", thread_times[i]);
    }
    printf("\n");

    total_time_ms = 1000*((long)end.tv_sec - (long)start.tv_sec) + ((long)end.tv_usec - (long)start.tv_usec)/1000;
    printf("%ld\n", total_time_ms);   //print total time

    return 0;
}