#ifndef ENCLONED_H
#define ENCLONED_H

#include <iostream>
#include <vector>
#include <memory> // shared_ptr
#include <chrono>
#include <fstream>
#include <filesystem>

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
namespace fs = std::filesystem;

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
        std::mutex daemonMtx;
        
        // file encryption key
        unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES]; // master key
        unsigned char subKey[64]; // derived subKey for index backup name encryption
        int loadEncryptionKey();
        int deriveSubKey();
        
    public:
        encloned();
        ~encloned();
        string daemonPath;
        static constexpr char TEMP_FILE_LOCATION[] = "/tmp/enclone/";
        
        std::mutex* daemonMtxPtr;

        int execLoop();
        unsigned char* const getKey();
        unsigned char* const getSubKey();

        void addWatch(string path, bool recursive); // needs mutex support
        void displayWatches();

};

#endif