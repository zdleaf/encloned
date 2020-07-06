#ifndef DB_H
#define DB_H

#include <iostream>
#include <sqlite3.h>

class DB {
    private:
        sqlite3 *db; // index handle

        void openDB();
        void closeDB();
        
        void initialiseTables(); // initialise tables on first run

    public:
        DB();
        ~DB();

        sqlite3* getDbPtr();
        virtual int execSQL(const char sql[]);
};

#endif