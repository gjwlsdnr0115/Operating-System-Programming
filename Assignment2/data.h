#ifndef HOMEWORK2_DATA_H
#define HOMEWORK2_DATA_H

class Data
{
public:
    Data(int dat_r, int dat_c);
    void init_array(int** chan, int rows, int cols);    //initialize 2d-array
    void set_data(int rows, int cols);      //set array data
    void pad_data();        //Zero-padding array with one space
    int dat_r, dat_c;       //row, and column of data
    int** chan1;            //channels
    int** chan2;
    int** chan3;

};


using namespace std;

void Data::init_array(int **chan, int rows, int cols) {
    for(int i=0; i<rows; i++) {
        chan[i] = new int[cols];        //initialize columns
    }
}
Data::Data(int dat_r, int dat_c) {
    this -> dat_r = dat_r;
    this -> dat_c = dat_c;
    chan1 = new int* [dat_r];
    init_array(chan1, dat_r, dat_c);
    chan2 = new int* [dat_r];
    init_array(chan2, dat_r, dat_c);
    chan3 = new int* [dat_r];
    init_array(chan3, dat_r, dat_c);
}
void Data::set_data(int rows, int cols) {
    for(int i=0; i<rows; i++) {         //input channel1 data
        for(int j=0; j<cols; j++){
            int n;
            scanf("%d", &n);
            chan1[i][j] = n;
        }
    }
    for(int i=0; i<rows; i++) {         //input channel2 data
        for(int j=0; j<cols; j++){
            int n;
            scanf("%d", &n);
            chan2[i][j] = n;
        }
    }for(int i=0; i<rows; i++) {        //input channel3 data
        for(int j=0; j<cols; j++){
            int n;
            scanf("%d", &n);
            chan3[i][j] = n;
        }
    }
}
void Data::pad_data() {
    int pad_r = dat_r + 2;      //padded data row
    int pad_c = dat_c + 2;      //padded data column
    vector<int**> pads;         //vector for padded data
    vector<int**> channels;     //vector for initial data
    int** pad1;                 //padded 2d-array
    int** pad2;
    int** pad3;
    pads.push_back(pad1);
    pads.push_back(pad2);
    pads.push_back(pad3);
    channels.push_back(chan1);
    channels.push_back(chan2);
    channels.push_back(chan3);

    for(int i=0; i<pads.size(); i++){       //initialize padded 2d-array
        pads[i] = new int*[pad_r];
        for(int j=0; j<pad_r; j++) {
            pads[i][j] = new int[pad_c]();
        }
    }
    for(int i=0; i<pads.size(); i++) {      //copy image to padded array
        for(int j=0; j<dat_r; j++) {
            for(int k=0; k<dat_c; k++) {
                pads[i][j+1][k+1] = channels[i][j][k];
            }
        }
    }
    chan1 = pads[0];    //reassign image
    chan2 = pads[1];
    chan3 = pads[2];
    dat_r = pad_r;      //update image size
    dat_c = pad_c;
}

#endif //HOMEWORK2_DATA_H
