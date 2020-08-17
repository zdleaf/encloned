#include <enclone/remote/Remote.h>

Remote::Remote(std::atomic_bool *runThreads, encloned* daemon)
{
    this->runThreads = runThreads;
    this->daemon = daemon;
    s3 = std::make_shared<S3>(runThreads, this);
}

void Remote::setPtr(std::shared_ptr<Watch> watch){
    this->watch = watch;
}

void Remote::execThread(){
    while(*runThreads){
        std::this_thread::sleep_for(std::chrono::seconds(10));
        //cout << "Remote: Calling Remote cloud storage..." << endl; cout.flush();
        uploadRemotes();
        deleteRemotes();
    }
}

encloned* Remote::getDaemon(){
    return daemon;
}

std::shared_ptr<Watch> Remote::getWatch(){
    return watch;
}

void Remote::uploadRemotes(){
    std::lock_guard<std::mutex> guard(mtx);
    s3->callAPI("upload");
}

string Remote::downloadRemotes(){
    std::lock_guard<std::mutex> guard(mtx);
    return s3->callAPI("download"); 
}

void Remote::deleteRemotes(){
    std::lock_guard<std::mutex> guard(mtx);
    s3->callAPI("delete");
}

bool Remote::queueForUpload(std::string path, std::string objectName, std::time_t modtime){
    std::lock_guard<std::mutex> guard(mtx);
    // call remotes
    return s3->enqueueUpload(path, objectName, modtime);
}

bool Remote::queueForDownload(std::string path, std::string objectName, std::time_t modtime, string targetPath){
    std::lock_guard<std::mutex> guard(mtx);
    // call remotes
    return s3->enqueueDownload(path, objectName, modtime, targetPath);
}

bool Remote::queueForDelete(std::string objectName){
    std::lock_guard<std::mutex> guard(mtx);
    // call remotes
    return s3->enqueueDelete(objectName);
}

void Remote::uploadSuccess(std::string path, std::string objectName, int remoteID){ // update fileIndex if upload to remote is succesful
    watch->uploadSuccess(path, objectName, remoteID);
}

string Remote::uploadNow(string path, string pathHash){
    return s3->callAPI("uploadNow|" + path + "|" + pathHash);
}

string Remote::listObjects(){
    mtx.lock();
    std::vector<string> objects;
    try {
        objects = s3->getObjects();
        if(objects.empty()){
            return "Remote: no files on remote S3 bucket\n";
        }
    } catch(const std::exception& e){ // listObjects may fail due invalid credentials etc
        return e.what();
    }
    mtx.unlock();
    std::ostringstream ss;
    for(string pathHash: objects){
        try {
            auto pair = watch->resolvePathHash(pathHash);
            ss << pair.first << " : " << watch->displayTime(pair.second) << " : " << pathHash << endl;
        } catch (std::out_of_range &error){ // unable to resolve
            continue;
        }
    }
    return ss.str();
}

string Remote::cleanRemote(){
    mtx.lock();
    std::vector<string> objects;
    try {
        objects = s3->getObjects();
        if(objects.empty()){
            return "Remote: no files on remote S3 bucket\n";
        }
    } catch(const std::exception& e){ // listObjects may fail due invalid credentials etc
        return e.what();
    }
    mtx.unlock();
    std::ostringstream ss;
    for(string pathHash: objects){
        try {
            auto pair = watch->resolvePathHash(pathHash);
        } catch (std::out_of_range &error){ // unable to resolve
            queueForDelete(pathHash);
            ss << "Remote: " << pathHash << " queued for deletion - no match for this entry found in fileIndex" << endl;
        }
    }
    return ss.str();
}