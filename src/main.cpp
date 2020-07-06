#include <enclone/enclone.h>

// g++ ./src/*.cpp -I./include -o main -std=c++17 -lstdc++fs

// boost 
// /usr/lib/x86_64-linux-gnu/
// /usr/include/boost/
// g++ *.cpp -I. -o main -std=c++17 -lstdc++fs -lboost_filesystem -lboost_system

int main(){
    enclone *e = new enclone();
/*     e->addWatch("/home/zach/enclone/notexist", true); // check non existent file
    e->addWatch("/home/zach/enclone/tmp", true); // recursive sadd
    e->addWatch("/home/zach/enclone/tmp/subdir", false); // check dir already added
    e->addWatch("/home/zach/enclone/tmp/file1", false); // check file already added */
    //e->displayWatches();
    e->execLoop();
}