#include <./Watch.h>

// g++ *.cpp -I. -o main -std=c++17 -lstdc++fs

int main(){

    Watch *p1 = new Watch("/mnt/g/Dropbox/QMUL/FinalProject/enclone");
    p1->listDir();

}