#include <enclone/enclone.h>

enclone::enclone(){ // constructor
    db = new DB();
    watch = new Watch(db);
    //socket = new Socket();
    // restore from DB here
}

enclone::~enclone(){ // destructor
    watch->execQueuedSQL(); // execute any pending SQL before closing
    // delete objects
    delete db; 
    delete watch;
    delete socket;
}

int enclone::execLoop(){
    while(1){
        cout << "Scanning for file changes..." << endl; cout.flush();
        watch->scanFileChange();
        watch->execQueuedSQL();
        std::this_thread::sleep_for(std::chrono::seconds(5));
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