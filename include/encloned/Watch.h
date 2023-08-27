#ifndef WATCH_H
#define WATCH_H

#include <exception>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
//#include <sys/inotify.h>
#include <algorithm>
#include <chrono>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// concurrency/multi-threading
#include <encloned/DB.h>
#include <encloned/Encryption.h>
#include <encloned/remote/Remote.h>

#include <atomic>
#include <mutex>
#include <thread>

using std::cout;
using std::endl;
using std::get;
using std::string;
namespace fs = std::filesystem;

class Remote;
class encloned;

struct FileVersion {
  std::time_t modtime;
  std::string pathHash;
  std::string fileHash;
  bool localExists = true;  // false if file has been deleted from local fs
  bool remoteExists = false;   // set flag once successfully uploaded to remote
  std::string remoteLocation;  // remote locations the file exists
};

class Watch {
 public:
  Watch(std::shared_ptr<DB> db, std::atomic_bool* runThreads, encloned* daemon);
  ~Watch();

  void setPtr(std::shared_ptr<Remote> remote);

  Watch(const Watch&) = delete;
  Watch& operator=(const Watch&) = delete;

  void execThread();

  void scanFileChange();
  void execQueuedSQL();

  string addWatch(string path, bool recursive);
  string delWatch(string path, bool recursive);
  void displayWatchDirs();
  void displayWatchFiles();

  string listLocal();
  string listWatchDirs();
  string listWatchFiles();

  string downloadFiles(string targetPath);  // download all
  string downloadFiles(string targetPath,
                       string pathOrHash);  // specify path or hash to download

  std::unordered_map<string, std::vector<FileVersion>>* getFileIndex();
  std::pair<string, std::time_t> resolvePathHash(string pathHash);
  // check a hash matches the stored filehash in fileIndex
  bool verifyHash(string pathHash, string fileHash) const;
  // update the index if a file was successfully uploaded
  void uploadSuccess(std::string path, std::string objectName, int remoteID);
  string restoreIndex(string arg);

  // helper functions
  string displayTime(std::time_t modtime) const;
  time_t fsLastMod(string path);  // get last mod time from file system

 private:
  std::unordered_map<string, bool>
      dirIndex;  // index of watched directories with bool recursive flag
  std::unordered_map<string, std::vector<FileVersion>>
      fileIndex;  // index of watched files, key = path, with a vector of
                  // different available file versions
  std::unordered_map<string, std::pair<string, std::time_t>>
      pathHashIndex;  // easily resolve pathHash to path and modtime

  std::shared_ptr<Remote> remote;  // pointer to Remote handler
  std::shared_ptr<DB> db;          // database handle
  encloned* daemon;                // ptr to main daemon class that spawned this

  // sql queue/bucket of queries to execute in batches
  std::stringstream sqlQueue;

  // concurrency/multi-threading
  std::mutex mtx;
  std::atomic_bool* runThreads;  // ptr to flag indicating if execThread should
                                 // loop or close down

  // file system watcher
  string addDirWatch(string path, bool recursive);
  string addFileWatch(string path);
  void addFileVersion(std::string path);

  string delDirWatch(string path, bool recursive);
  string delFileWatch(string path);

  std::time_t getLastModFromIdx(std::string path);

  // backup index to remote storage methods
  string indexBackupName;
  std::time_t indexLastMod;
  void deriveIdxBackupName();
  void indexBackup();

  // restore from DB on daemon start
  void restoreDB();
  void restoreFileIdx();
  void restoreDirIdx();
  void restoreIdxBackupName();
};

#endif