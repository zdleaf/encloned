#include <enclone/enclone.h>

enclone::enclone(){ // constructor
    runThreads = true;

    db = new DB();
    watch = new Watch(db, &runThreads);
    socket = new Socket(&runThreads);
    s3 = new S3(&runThreads);

}

enclone::~enclone(){ // destructor
    // delete objects
    delete db; 
    delete watch;
    delete socket;
}

int enclone::execLoop(){
    cout << "Starting S3 thread..." << endl;
    std::thread s3Thread{&S3::execThread, s3}; // start a thread scanning for filesystem changes
    s3Thread.detach();                               // detach thread, we not want to wait for it to finish before continuing. execThread() loops until runThreads == false;

    cout << "Starting watch thread..." << endl;
    std::thread watchThread{&Watch::execThread, watch}; // start a thread scanning for filesystem changes
    watchThread.detach();                               // detach thread, we not want to wait for it to finish before continuing. execThread() loops until runThreads == false;

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