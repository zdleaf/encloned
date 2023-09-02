#ifndef DB_H
#define DB_H

#include <sqlite3.h>

#include <filesystem>
#include <iostream>

// concurrency/multi-threading
#include <atomic>
#include <mutex>
#include <thread>

namespace fs = std::filesystem;

using std::cout;
using std::endl;

class DB {
 private:
  const char* DATABASE_LOCATION = "index.db";

  sqlite3* db;  // index handle

  std::mutex mtx;

  void initialiseTables();  // initialise tables on first run
  void backupProgress(int leftToCopy, int totalToCopy);

 public:
  DB();
  ~DB();

  void openDB();
  void closeDB();

  sqlite3* getDbPtr();
  const char* getDbLocation();

  virtual int execSQL(const char sql[]);
  int backupDB(const char* backupFilename);  // complete an online backup of an
                                             // open database to backupFilename
};

#endif
