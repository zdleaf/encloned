#ifndef DB_H
#define DB_H

#include <iostream>
#include <sqlite3.h>
#include <filesystem>

namespace fs = std::filesystem;

using std::cout;
using std::endl;

class DB {
    private:
        const char* DATABASE_LOCATION = "index.db";
        
        sqlite3 *db; // index handle

        void openDB();
        void closeDB();
        
        void initialiseTables(); // initialise tables on first run

    public:
        DB();
        ~DB();

        sqlite3* getDbPtr();
        const char* getDbLocation();
        virtual int execSQL(const char sql[]);
};

#endif