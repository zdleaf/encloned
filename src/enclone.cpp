#include <enclone/enclone.h>

enclone::enclone(){
    db = new DB();
}

enclone::~enclone(){
    // destructor
    delete db; // delete db object
}

void enclone::addWatch(string path){
    // also need a flag for recursive add or not
    std::shared_ptr<Watch> dir(new Watch(path));
    watchDirs.push_back(dir);
}

