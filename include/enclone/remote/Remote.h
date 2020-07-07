#ifndef REMOTE_H
#define REMOTE_H

#include <iostream>
#include <string>
#include <memory>

#include <enclone/remote/S3.h>

// stub class - this will be the middle man for handling multiple remotes at one time, e.g. multiple upload to different locations
// needs to store which remotes are available and turned on for upload

class Remote{
    private:
        // available remotes
        std::shared_ptr<S3> s3;

        // concurrency/multi-threading
        std::mutex mtx;
        std::atomic_bool *runThreads; // ptr to flag indicating if execThread should loop or close down

    public:
        Remote(std::atomic_bool *runThreads);

        void execThread();
        void callRemotes();

        bool queueForUpload(std::string path, std::string objectName);
        bool queueForDelete(std::string objectName);
};

#endif