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

// c++17 std::filesystem
// linux filesystem changes - inotify

/* struct File {
    string path;
    fs::file_time_type modtime;
    int size;
    string hash;

    bool operator==(const File& rhs) { return path == rhs.path; }; // override for unordered_set
    size_t operator()(const File& elementToHash) const noexcept {
        size_t hash = elementToHash.path;
        return hash;
    };
}; */

class Watch {
    private:
        std::unordered_set<string> watchDirs;
        std::unordered_map<string, fs::file_time_type> watchFiles;

        void addDirWatch(string &path, bool recursive);
        void addFileWatch(string path);

    public:
        Watch();

        void addWatch(string path, bool recursive);

        void listDir(string path);
        void fileAttributes(const fs::path& path);

        void displayWatchDirs();
        void displayWatchFiles();

        string displayTime(fs::file_time_type modtime) const;

};

#endif