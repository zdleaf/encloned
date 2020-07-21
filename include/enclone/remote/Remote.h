#ifndef REMOTE_H
#define REMOTE_H

#include <iostream>
#include <string>
#include <memory>

#include <enclone/remote/S3.h>
#include <enclone/Watch.h>

// stub class - this will be the middle man for handling multiple remotes at one time, e.g. multiple upload to different locations
// needs to store which remotes are available and turned on for upload

class S3;
class Watch;

class Remote{
    private:
        // object pointers
        std::shared_ptr<Watch> watch; 

        // available remotes
        std::shared_ptr<S3> s3;

        // concurrency/multi-threading
        std::mutex mtx;
        std::atomic_bool *runThreads; // ptr to flag indicating if execThread should loop or close down

    public:
        Remote(std::atomic_bool *runThreads);
        void setPtr(std::shared_ptr<Watch> watch);

        void execThread();
        void uploadRemotes();
        string downloadRemotes();

        void uploadSuccess(std::string path, std::string objectName, int remoteID); // update fileIndex if upload to remote is succesfull, returns the remoteID it was succesfully uploaded to

        bool queueForUpload(std::string path, std::string objectName, std::time_t modtime);
        bool queueForDownload(std::string path, std::string objectName, std::time_t modtime);
        bool queueForDelete(std::string objectName);

        string listObjects();
};

#endif