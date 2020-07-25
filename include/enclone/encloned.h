#ifndef ENCLONED_H
#define ENCLONED_H

#include <iostream>
#include <vector>
#include <memory> // shared_ptr
#include <chrono>
#include <fstream>

// concurrency/multi-threading
#include <thread>
#include <mutex>
#include <atomic>

// get path of this executable
#include <libgen.h>         // dirname
#include <unistd.h>         // readlink
#include <linux/limits.h>   // PATH_MAX

#include <enclone/DB.h>
#include <enclone/Watch.h>
#include <enclone/Socket.h>
#include <enclone/remote/Remote.h>

namespace io = boost::asio;

class Watch;
class Socket;
class DB;
class Remote;

class encloned{
    private:
        std::shared_ptr<DB> db; // database handle
        std::shared_ptr<Watch> watch; // watch file/directory class
        std::shared_ptr<Socket> socket; // local unix domain socket for enclone-config
        std::shared_ptr<Remote> remote; // remote backend handler

        std::atomic<bool> runThreads; // flag to indicate whether detached threads should continue to run

        // file encryption key
        unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
        int loadEncryptionKey();
        
    public:
        encloned();
        ~encloned();
        string daemonPath;

        int execLoop();
        unsigned char* getKey();

        void addWatch(string path, bool recursive); // needs mutex support
        void displayWatches();

};

#endif