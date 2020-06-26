#ifndef WATCH_H
#define WATCH_H

#include <iostream>
#include <filesystem>
#include <string>
#include <sstream>
//#include <sys/inotify.h>
#include <chrono>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#include <enclone/DB.h>

using std::string;
using std::cout;
using std::endl;
namespace fs = std::filesystem;

class Watch {
    private:
        std::unordered_map<string, bool> dirIndex;                  // index of watched directories with bool recursive flag
        std::unordered_map<string, std::time_t> fileIndex;          // index of watched files with last mod time

        DB *db;                         // database handle
        std::stringstream sqlQueue;     // sql queue/bucket of queries to execute in batches

        void addDirWatch(string path, bool recursive);
        void addFileWatch(string path);

        void restoreDB();

        string displayTime(std::time_t modtime) const;

        void listDir(string path);
        void fileAttributes(const fs::path& path);

    public:
        Watch(DB *db);
        ~Watch();

        void addWatch(string path, bool recursive);
        void scanFileChange();

        void execQueuedSQL();

        void displayWatchDirs();
        void displayWatchFiles();
};

#endif