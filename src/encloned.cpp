#include <enclone/encloned.h>

int main(){
    encloned daemon;
    daemon.execLoop();
}

encloned::encloned(){ // constructor
    runThreads = true;
    db = std::make_shared<DB>();
    socket = std::make_shared<Socket>(&runThreads);
    remote = std::make_shared<Remote>(&runThreads);
    watch = std::make_shared<Watch>(db, &runThreads);
    remote->setPtr(watch);
    watch->setPtr(remote);
    socket->setPtr(watch);
    socket->setPtr(remote);
    Encryption::initSodium();
}

encloned::~encloned(){ // destructor
    // delete objects
}

void encloned::generateKey(){
    crypto_secretstream_xchacha20poly1305_keygen(key);
    cout << "Encryption: Generated encryption key" << endl;
}

int encloned::execLoop(){
    cout << "Starting Watch thread..." << endl; cout.flush();
    std::thread watchThread{&Watch::execThread, watch}; // start a thread scanning for filesystem changes
    watchThread.detach();                               // detach thread, we not want to wait for it to finish before continuing. execThread() loops until runThreads == false;

    cout << "Starting Remote thread..." << endl; cout.flush();
    std::thread remoteThread{&Remote::execThread, remote}; // start a thread scanning for filesystem changes
    remoteThread.detach();                               // detach thread, we not want to wait for it to finish before continuing. execThread() loops until runThreads == false;

    cout << "Starting Socket thread..." << endl; cout.flush();
    std::thread socketThread{&Socket::execThread, socket}; // start a thread scanning for filesystem changes
    socketThread.detach();                               // detach thread, we not want to wait for it to finish before continuing. execThread() loops until runThreads == false;

    while(1){
        // do nothing while watch thread is running
    }
    return 0;
}

void encloned::addWatch(string path, bool recursive){
    watch->addWatch(path, recursive);
}

void encloned::displayWatches(){
    watch->displayWatchDirs();
    watch->displayWatchFiles();
}