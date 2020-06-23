#ifndef ENCLONE_H
#define ENCLONE_H

#include <iostream>
#include <vector>
#include <memory> // shared_ptr

#include <enclone/DB.h>
#include <enclone/Watch.h>

class enclone {
    private:
        DB *db; // database handle
        Watch *watch; // watch file/directory class
        
    public:
        enclone();
        ~enclone();

        void addWatch(string path, bool recursive);
};

#endif