#include <iostream>
#include <string>
#include <vector>
#include <sys/time.h>
#include "data.h"

using namespace std;

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
int **convolution(Data img, Data fil) {         //perform convolution
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
    return convoluted;
}

int main(int argc, char* argv[]) {
    long total_time_ms;     //total operation time
    struct timeval start, end;  //start, end point of time

    int fil_n, fil_r, fil_c;
    scanf("%d" "%d" "%d", &fil_n, &fil_r, &fil_c);  //input filter number, row, column

    vector <Data*> fil_vec;     //make filter vector
    for(int i=0; i<fil_n; i++) {
        Data* f = new Data(fil_r, fil_c);
        f->set_data(fil_r, fil_c);      //input filter data
        fil_vec.push_back(f);
    }

    int img_r, img_c;
    scanf("%d" "%d", &img_r, &img_c);   //input image row, column

    Data img = Data(img_r, img_c);
    img.set_data(img_r, img_c);     //set image data
    img.pad_data();     //pad image

    int res_r = img.dat_r - (fil_r - 1);    //result data row
    int res_c = img.dat_c - (fil_c - 1);    //result data column

    gettimeofday(&start, NULL); //start time
    for(int i=0; i<fil_n; i++) {
        int** result = convolution(img, *fil_vec[i]);   //convolution result
        for(int j=0; j<res_r; j++) {        //print result
            for(int k=0; k<res_c; k++) {
                printf("%d ", result[j][k]);
            }
            printf("\n");
        }
        printf("\n");
    }

    gettimeofday(&end, NULL);   //end time
    total_time_ms = 1000*((long)end.tv_sec - (long)start.tv_sec) + ((long)end.tv_usec - (long)start.tv_usec)/1000;    //elapsed time in milliseconds
    printf("%ld\n", total_time_ms);   //print total time

    return 0;
}