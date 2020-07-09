#include <enclone/remote/Remote.h>

Remote::Remote(std::atomic_bool *runThreads)
{
    this->runThreads = runThreads;
    s3 = std::make_shared<S3>(runThreads, this);
}

void Remote::setPtr(std::shared_ptr<Watch> watch){
    this->watch = watch;
}

void Remote::execThread(){
    while(*runThreads){
        std::this_thread::sleep_for(std::chrono::seconds(10));
        cout << "Remote: Calling Remote cloud storage..." << endl; cout.flush();
        callRemotes();
    }
}

void Remote::callRemotes(){
    std::lock_guard<std::mutex> guard(mtx);
    s3->callAPI();
}

bool Remote::queueForUpload(std::string path, std::string objectName){
    std::lock_guard<std::mutex> guard(mtx);
    // call remotes
    return s3->enqueueUpload(path, objectName);
}

bool Remote::queueForDelete(std::string objectName){
    std::lock_guard<std::mutex> guard(mtx);
    // call remotes
    return s3->enqueueDelete(objectName);
}

void Remote::uploadSuccess(std::string path, std::string objectName, int remoteID){ // update fileIndex if upload to remote is succesfull
    watch->uploadSuccess(path, objectName, remoteID);
}