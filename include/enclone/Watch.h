#ifndef WATCH_H
#define WATCH_H

#include <iostream>
#include <filesystem>
#include <string>
//#include <sys/inotify.h>
#include <chrono>
#include <vector>

using std::string;
using std::cout;
using std::endl;
namespace fs = std::filesystem;

// c++17 std::filesystem
// linux filesystem changes - inotify

struct File {
  string path;
  fs::file_time_type modtime;
  int size;
  string hash;
};

class Watch {
    private:
        std::vector<string> watchDirs;
        std::vector<string> watchFiles;

    public:
        Watch();

        void addWatch(string path, bool recursive);
        void listDir(string path);
        void fileAttributes(const fs::path& path);

};

#endif