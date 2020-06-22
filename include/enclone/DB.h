#ifndef DB_H
#define DB_H

#include <iostream>
#include <sqlite3.h>

class DB {
    private:
        sqlite3 *db; // index handle

    public:
        DB();
        
        void openDB();
        void closeDB();
};

#endif