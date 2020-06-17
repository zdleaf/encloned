#include <enclone/Watch.h>

// g++ ./src/*.cpp -I./include -o main -std=c++17 -lstdc++fs

// boost 
// /usr/lib/x86_64-linux-gnu/
// /usr/include/boost/
// g++ *.cpp -I. -o main -std=c++17 -lstdc++fs -lboost_filesystem -lboost_system

int main(){

    Watch *p1 = new Watch("/mnt/g/Dropbox/QMUL/FinalProject/enclone");
    p1->listDir();

}