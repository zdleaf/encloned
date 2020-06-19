#ifndef INDEX_H
#define INDEX_H

#include <iostream>
#include <sqlite3.h>

class FileIndex {
    private:
        sqlite3 *db; // index handle

    public:
        FileIndex();
        
        void openIndex();
        void closeIndex();
};

#endif