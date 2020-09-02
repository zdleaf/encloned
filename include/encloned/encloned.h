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

#include <encloned/DB.h>
#include <encloned/Watch.h>
#include <encloned/Socket.h>
#include <encloned/remote/Remote.h>

namespace io = boost::asio;
namespace fs = std::filesystem;

class Watch;
class Socket;
class DB;
class Remote;

class encloned{
    private:
        std::shared_ptr<DB> db;         // database handle
        std::shared_ptr<Watch> watch;   // watch file/directory class
        std::shared_ptr<Socket> socket; // local unix domain socket for enclone
        std::shared_ptr<Remote> remote; // remote backend handler

        std::atomic<bool> runThreads; // flag to indicate whether detached threads should continue to run
        std::mutex daemonMtx;
        
        // file encryption key
        unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES]; // master key
        unsigned char subKey[64]; // derived subKey for index backup name encryption
        string subKey_b64;
        int loadEncryptionKey();
        int deriveSubKey();
        
    public:
        encloned();
        ~encloned();
        string daemonPath;
        static constexpr char TEMP_FILE_LOCATION[] = "/tmp/enclone/"; // path to store temporary encrypted files before upload, and downloaded files before decryption
        
        std::mutex* daemonMtxPtr;

        int execLoop();
        unsigned char* const getKey();
        string const getSubKey_b64();

        void addWatch(string path, bool recursive); // needs mutex support
        void displayWatches();

};

#endif