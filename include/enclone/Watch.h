#ifndef WATCH_H
#define WATCH_H

#include <iostream>
#include <filesystem>
#include <string>
//#include <sys/inotify.h>
#include <chrono>

using std::string;
using std::cout;
using std::endl;
namespace fs = std::filesystem;

// c++17 std::filesystem
// linux filesystem changes - inotify

class Watch {
    private:
        string path;

    public:
        Watch(string path);
        void listDir();
        void fileAttributes(const fs::path& path);

};

#endif