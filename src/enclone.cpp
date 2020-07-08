#include <enclone/enclone.h>

enclone::enclone(){ // constructor
    runThreads = true;
    db = std::make_shared<DB>();
    socket = std::make_shared<Socket>(&runThreads);
    remote = std::make_shared<Remote>(&runThreads, getPtr());
    watch = std::make_shared<Watch>(db, &runThreads, getPtr());
}

enclone::~enclone(){ // destructor
    // delete objects
}

std::shared_ptr<enclone> enclone::getPtr(){
        return shared_from_this();
}

std::shared_ptr<Remote> enclone::getRemotePtr(){
    return remote;
}

std::shared_ptr<Watch> enclone::getWatchPtr(){
    return watch;
}

int enclone::execLoop(){
    cout << "Starting Watch thread..." << endl;
    std::thread watchThread{&Watch::execThread, watch}; // start a thread scanning for filesystem changes
    watchThread.detach();                               // detach thread, we not want to wait for it to finish before continuing. execThread() loops until runThreads == false;

    cout << "Starting Remote thread..." << endl;
    std::thread remoteThread{&Remote::execThread, remote}; // start a thread scanning for filesystem changes
    remoteThread.detach();                               // detach thread, we not want to wait for it to finish before continuing. execThread() loops until runThreads == false;

/*     cout << "Starting socket thread..." << endl;
    std::thread socketThread{&Socket::execThread, socket}; // start a thread scanning for filesystem changes
    socketThread.detach();                               // detach thread, we not want to wait for it to finish before continuing. execThread() loops until runThreads == false; */

    while(1){
        // do nothing while watch thread is running
    }
    return 0;
}

void enclone::addWatch(string path, bool recursive){
    watch->addWatch(path, recursive);
}

void enclone::displayWatches(){
    watch->displayWatchDirs();
    watch->displayWatchFiles();
}