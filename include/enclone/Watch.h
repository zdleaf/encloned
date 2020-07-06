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

// concurrency/multi-threading
#include <thread>
#include <mutex>
#include <atomic>

#include <enclone/DB.h>
#include <enclone/Encryption.h>
#include <enclone/remote/Remote.h>

using std::string;
using std::cout;
using std::endl;
namespace fs = std::filesystem;

class Watch {
    public:
        Watch(std::shared_ptr<DB> db, std::atomic_bool *runThreads, std::shared_ptr<Remote> remote);
        ~Watch();

        // delete copy constructors - this class should not be copied
        Watch(const Watch&) = delete;
        Watch& operator=(const Watch&) = delete;

        void execThread();

        void addWatch(string path, bool recursive);
        void scanFileChange();
        void execQueuedSQL();

        void displayWatchDirs();
        void displayWatchFiles();

    private:
        std::unordered_map<string, bool> dirIndex;                  // index of watched directories with bool recursive flag
        std::unordered_map<string, std::time_t> fileIndex;          // index of watched files with last mod time
        std::unordered_map<string, string> pathHashIndex;                // stores the path of a file with it's computed filename hash

        std::shared_ptr<Remote> remote; // pointer to Remote handler

        std::shared_ptr<DB> db;         // database handle
        std::stringstream sqlQueue;     // sql queue/bucket of queries to execute in batches

        // concurrency/multi-threading
        std::mutex mtx;
        std::atomic_bool *runThreads; // ptr to flag indicating if execThread should loop or close down

        void addDirWatch(string path, bool recursive);
        void addFileWatch(string path);

        void restoreDB();
        void restoreFileIdx();
        void restoreDirIdx();

        string displayTime(std::time_t modtime) const;

        void listDir(string path);
        void fileAttributes(const fs::path& path);
};

#endif