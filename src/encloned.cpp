#include <enclone/encloned.h>

int main(){
    if(!fs::exists("key")){
        cout << "Encryption key does not exist - please run \"enclone --generate-key\" first" << endl;
        return 1;
    }
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

int encloned::execLoop(){
    if(loadEncryptionKey() != 0){
        return 1;
    }

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

int encloned::loadEncryptionKey(){
    if(fs::file_size("key") != crypto_secretstream_xchacha20poly1305_KEYBYTES){
        cout << "Error loading encryption key - key size is incorrect" << endl;
        return 1;         
    }
    std::streampos size;
    char memblock[crypto_secretstream_xchacha20poly1305_KEYBYTES];

    std::ifstream file ("key", std::ios::in|std::ios::binary|std::ios::ate);
    if (file.is_open())
    {
        size = file.tellg();
        file.seekg(0, std::ios::beg);
        file.read(memblock, size);
        file.close();

        memcpy(key, memblock, sizeof memblock); // copy into the key member variable
        cout << "Successfully loaded encryption key from key file" << endl;

        //delete[] memblock;
    }
    else { 
        cout << "Error loading encryption key - please check \"key\" and try again" << endl;
        return 1; 
    }
    return 0;
}