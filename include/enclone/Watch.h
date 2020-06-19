#ifndef WATCH_H
#define WATCH_H

#include <iostream>
#include <filesystem>
#include <string>
//#include <sys/inotify.h>
#include <chrono>
#include <sqlite3.h>

using std::string;
using std::cout;
using std::endl;
namespace fs = std::filesystem;

// c++17 std::filesystem
// linux filesystem changes - inotify

class Watch {
    private:
        string path;
        sqlite3 *db; // index handle

    public:
        Watch(string path);
        
        void openIndex();
        void closeIndex();

        void listDir();
        void fileAttributes(const fs::path& path);

};

#endif