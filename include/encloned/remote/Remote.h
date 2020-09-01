#ifndef REMOTE_H
#define REMOTE_H

#include <iostream>
#include <string>
#include <memory>
#include <chrono>

#include <encloned/remote/S3.h>
#include <encloned/Watch.h>
#include <encloned/encloned.h>

class S3;
class Watch;
class encloned;

class Remote{
    private:
        // object pointers
        encloned* daemon; // ptr to main daemon class that spawned this
        std::shared_ptr<Watch> watch; 

        // available remotes
        std::shared_ptr<S3> s3;

        // concurrency/multi-threading
        std::mutex mtx;
        std::atomic_bool *runThreads; // ptr to flag indicating if execThread should loop or close down

    public:
        Remote(std::atomic_bool* runThreads, encloned* daemon);
        void setPtr(std::shared_ptr<Watch> watch);
        encloned* getDaemon();
        std::shared_ptr<Watch> getWatch();

        void execThread();

        bool queueForUpload(std::string path, std::string objectName, std::time_t modtime);
        bool queueForDownload(std::string path, std::string objectName, std::time_t modtime, string targetPath);
        bool queueForDelete(std::string objectName);

        void uploadRemotes();
        string uploadNow(string path, string pathHash);
        void uploadSuccess(std::string path, std::string objectName, int remoteID); // update fileIndex if upload to remote is succesfull, returns the remoteID it was succesfully uploaded to
        string downloadRemotes();
        string downloadNow(string pathHash, string target);
        void deleteRemotes();

        string listObjects();
        std::vector<string> getObjects();
        std::unordered_map<string, string> getObjectMap();
        string cleanRemote();
        
};

#endif