#ifndef WATCH_H
#define WATCH_H

#include <iostream>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;
using namespace std;

// c++17 std::filesystem

class Watch {
    private:
        string path;

    public:
        Watch(string path);
        void listDir();

};

#endif