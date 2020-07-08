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
        cout << "Remote: Calling Remote cloud storage..." << endl; cout.flush();
        callRemotes();
        std::this_thread::sleep_for(std::chrono::seconds(15));
    }
}

void Remote::callRemotes(){
    std::lock_guard<std::mutex> guard(mtx);
    s3->callAPI();
}

bool Remote::queueForUpload(std::string path, std::string objectName){
    std::lock_guard<std::mutex> guard(mtx);
    // call remotes
    return s3->queueForUpload(path, objectName);
}

bool Remote::queueForDelete(std::string objectName){
    std::lock_guard<std::mutex> guard(mtx);
    // call remotes
    return s3->queueForDelete(objectName);
}

void Remote::uploadSuccess(std::string path, std::string objectName, int remoteID){ // update fileIndex if upload to remote is succesfull
    watch->uploadSuccess(path, objectName, remoteID);
}