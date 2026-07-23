#include <opencv2/core.hpp>
#include <iostream>
using namespace cv;
using namespace std;
int main() {
    Mat m(2, 2, CV_32F);
    m.at<float>(0,0) = 0;
    m.at<float>(0,1) = 2;
    m.at<float>(1,0) = -2;
    m.at<float>(1,1) = 10;
    
    Mat negM = -m;
    Mat expM;
    exp(negM, expM);
    
    Mat sig = 1.0f / (1.0f + expM);
    
    cout << "Original:" << endl << m << endl;
    cout << "Sigmoid:" << endl << sig << endl;
    return 0;
}
