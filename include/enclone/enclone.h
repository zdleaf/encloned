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
        std::vector<std::shared_ptr<Watch>> watchDirs; // vector of shared_ptrs to directories to watch
        
    public:
        enclone();
        ~enclone();
        void addWatch(string path);
};

#endif