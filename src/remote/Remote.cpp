#include <enclone/remote/Remote.h>

Remote::Remote(std::atomic_bool *runThreads, std::shared_ptr<enclone> encloneInstance)
{
    this->encloneInstance = encloneInstance;
    this->runThreads = runThreads;
}

void Remote::execThread(){

    // all objects have been created, set correct pointers
    s3 = std::make_shared<S3>(runThreads, encloneInstance->getRemotePtr());
    this->watch = encloneInstance->getWatchPtr();

    while(*runThreads){
        cout << "Remote: Calling Remote cloud storage..." << endl; cout.flush();
        callRemotes();
        std::this_thread::sleep_for(std::chrono::seconds(30));
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
    auto fileIndex = watch->getFileIndex(); // get ptr to fileIndex
    auto fileVersionVector = fileIndex->at(path);
    // set the remoteExists flag for correct entry in fileIndex
    for(auto it = fileVersionVector.rbegin(); it != fileVersionVector.rend(); ++it){ // iterate in reverse, most likely the last entry is the one we're looking for
        if(it->pathhash == objectName){
            it->remoteExists = true;
        }
    }
    // also add remoteID to list of remotes it's been uploaded to e.g. remoteLocation
}