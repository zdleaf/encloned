#include <enclone/enclone.h>

enclone::enclone(){ // constructor
    db = new DB();
    watch = new Watch();
    //socket = new Socket();
    // restore from DB here
}

enclone::~enclone(){ // destructor
    // delete objects
    delete db; 
    delete watch;
    delete socket;
    // save to DB here
}

int enclone::execLoop(){
    while(1){
        cout << "Scanning for file changes..." << endl; cout.flush();
        watch->scanFileChange();
        sleep(5);
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