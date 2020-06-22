#ifndef DB_H
#define DB_H

#include <iostream>
#include <sqlite3.h>

class DB {
    private:
        sqlite3 *db; // index handle

    public:
        DB();
        ~DB();

        void openDB();
        void closeDB();
        void execSQL(const char sql[]);
        void initialiseTables(); // initialise tables on first run
};

#endif