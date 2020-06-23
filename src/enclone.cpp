#include <enclone/enclone.h>

enclone::enclone(){ // constructor
    db = new DB();
    watch = new Watch();
}

enclone::~enclone(){ // destructor
    delete db; // delete db object
}

void enclone::addWatch(string path, bool recursive){
    watch->addWatch(path, recursive);
}

