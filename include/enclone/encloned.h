#ifndef ENCLONED_H
#define ENCLONED_H

#include <iostream>
#include <vector>
#include <memory> // shared_ptr
#include <chrono>

// concurrency/multi-threading
#include <thread>
#include <mutex>
#include <atomic>

#include <enclone/DB.h>
#include <enclone/Watch.h>
#include <enclone/Socket.h>
#include <enclone/remote/Remote.h>

namespace io = boost::asio;

class Watch;

class encloned{
    private:
        std::shared_ptr<DB> db; // database handle
        std::shared_ptr<Watch> watch; // watch file/directory class
        std::shared_ptr<Socket> socket; // local unix domain socket for enclone-config
        std::shared_ptr<Remote> remote; // remote backend handler

        std::atomic<bool> runThreads; // flag to indicate whether detached threads should continue to run
        
    public:
        encloned();
        ~encloned();

        int execLoop();

        void addWatch(string path, bool recursive); // needs mutex support
        void displayWatches();
};

#endif