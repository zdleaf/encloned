#ifndef WATCH_H
#define WATCH_H

#include <iostream>
#include <filesystem>
#include <string>
#include <sstream>
#include <exception>
//#include <sys/inotify.h>
#include <chrono>
#include <vector>
#include <tuple>
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
using std::get;
namespace fs = std::filesystem;

class Remote;

struct FileVersion {
    std::time_t modtime;
    std::string pathhash;
    std::string filehash;
    bool localExists = true; // false if file has been deleted from local filesystem
    bool remoteExists = false; // set flag once successfully uploaded to remote
    std::string remoteLocation; // remote locations the file exists
};

class Watch {
    public:
        Watch(std::shared_ptr<DB> db, std::atomic_bool *runThreads);
        ~Watch();

        void setPtr(std::shared_ptr<Remote> remote);

        // delete copy constructors - this class should not be copied
        Watch(const Watch&) = delete;
        Watch& operator=(const Watch&) = delete;

        void execThread();

        std::unordered_map<string, std::vector<FileVersion>>* getFileIndex();

        string addWatch(string path, bool recursive);
        void scanFileChange();
        void execQueuedSQL();
        
        void displayWatchDirs();
        void displayWatchFiles();

        string displayTime(std::time_t modtime) const;

        string listLocal();
        string listWatchDirs();
        string listWatchFiles();
        std::pair<string, std::time_t> resolvePathHash(string pathHash);

        void uploadSuccess(std::string path, std::string objectName, int remoteID);

    private:
        std::unordered_map<string, bool> dirIndex;                  // index of watched directories with bool recursive flag
        std::unordered_map<string, std::vector<FileVersion>> fileIndex;   // index of watched files, key = path, with a vector of different available file versions
        std::unordered_map<string, std::pair<string, std::time_t>> pathHashIndex; // easily resolve pathHash to path and modtime

        std::shared_ptr<Remote> remote; // pointer to Remote handler
        std::shared_ptr<DB> db;         // database handle

        std::stringstream sqlQueue;     // sql queue/bucket of queries to execute in batches

        // concurrency/multi-threading
        std::mutex mtx;
        std::atomic_bool *runThreads; // ptr to flag indicating if execThread should loop or close down

        string addDirWatch(string path, bool recursive);
        string addFileWatch(string path);
        void addFileVersion(std::string path);
        std::time_t getLastModTime(std::string path);

        void restoreDB();
        void restoreFileIdx();
        void restoreDirIdx();

        void listDir(string path);
        void fileAttributes(const fs::path& path);
};

#endif