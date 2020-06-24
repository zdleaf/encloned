#ifndef ENCLONE_H
#define ENCLONE_H

#include <iostream>
#include <vector>
#include <memory> // shared_ptr

#include <enclone/DB.h>
#include <enclone/Watch.h>
#include <enclone/Socket.h>

namespace io = boost::asio;

class enclone {
    private:
        DB *db; // database handle
        Watch *watch; // watch file/directory class
        Socket *socket; // local unix domain socket for enclone-config
        
    public:
        enclone();
        ~enclone();

        int execLoop();

        void addWatch(string path, bool recursive);
        void displayWatches();
};

#endif