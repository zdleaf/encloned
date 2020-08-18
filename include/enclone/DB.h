#ifndef DB_H
#define DB_H

#include <iostream>
#include <sqlite3.h>
#include <filesystem>

// concurrency/multi-threading
#include <thread>
#include <mutex>
#include <atomic>

namespace fs = std::filesystem;

using std::cout;
using std::endl;

class DB {
    private:
        const char* DATABASE_LOCATION = "index.db";
        
        sqlite3 *db; // index handle

        std::mutex mtx;
        
        void initialiseTables(); // initialise tables on first run

    public:
        DB();
        ~DB();

        void openDB();
        void closeDB();

        sqlite3* getDbPtr();
        const char* getDbLocation();
        virtual int execSQL(const char sql[]);
};

#endif