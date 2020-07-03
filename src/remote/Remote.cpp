#include <enclone/remote/Remote.h>

Remote::Remote(std::atomic_bool *runThreads)
{
    this->runThreads = runThreads;
    s3 = std::make_shared<S3>(runThreads);
}

void Remote::execThread(){
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