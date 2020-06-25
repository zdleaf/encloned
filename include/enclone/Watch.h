#ifndef WATCH_H
#define WATCH_H

#include <iostream>
#include <filesystem>
#include <string>
//#include <sys/inotify.h>
#include <chrono>
#include <vector>
#include <unordered_set>
#include <unordered_map>

using std::string;
using std::cout;
using std::endl;
namespace fs = std::filesystem;

class Watch {
    private:
        std::unordered_map<string, bool> dirIndex;                  // index of watched directories with bool recursive flag
        std::unordered_map<string, fs::file_time_type> fileIndex;   // index of watched files with last mod time

        void addDirWatch(string path, bool recursive);
        void addFileWatch(string path);

        void listDir(string path);
        void fileAttributes(const fs::path& path);

        string displayTime(fs::file_time_type modtime) const;

    public:
        Watch();

        void addWatch(string path, bool recursive);
        void scanFileChange();

        void displayWatchDirs();
        void displayWatchFiles();
};

#endif