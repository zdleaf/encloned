#include <enclone/enclone.h>

enclone::enclone(){ // constructor
    runThreads = true;

    db = new DB();
    watch = new Watch(db, &runThreads);
    //socket = new Socket();
}

enclone::~enclone(){ // destructor
    // delete objects
    delete db; 
    delete watch;
    delete socket;
}

int enclone::execLoop(){
    cout << "Starting watch thread..." << endl;
    std::thread watchThread{&Watch::execThread, watch}; // start a thread scanning for filesystem changes
    watchThread.detach();                               // detach thread, we not want to wait for it to finish before continuing. execThread() loops until runThreads == false;
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