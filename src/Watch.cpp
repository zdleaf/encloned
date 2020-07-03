#include <enclone/Watch.h>

Watch::Watch(std::shared_ptr<DB> db, std::atomic_bool *runThreads, std::shared_ptr<Remote> remote){ // constructor
    this->db = db; // set DB handle
    this->runThreads = runThreads;
    this->remote = remote;
}

Watch::~Watch(){ // destructor
    execQueuedSQL(); // execute any pending SQL before closing
}

void Watch::execThread(){
    while(*runThreads){
        cout << "Watch: Scanning for file changes..." << endl; cout.flush(); 
        scanFileChange();
        execQueuedSQL();
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void Watch::addWatch(string path, bool recursive){
    fs::file_status s = fs::status(path);
    if(!fs::exists(s)){                 // file/directory does not exist
        std::cout << "Watch: " << path << " does not exist" << endl;
    } else if(fs::is_directory(s)){     // adding a directory to watch
        addDirWatch(path, recursive);
    } else if(fs::is_regular_file(s)){  // adding a regular file to watch
        addFileWatch(path);
    } else {                            // any other file type, e.g. IPC pipe
        std::cout << "Watch: " << path << " is not a valid directory/file" << endl;
    }
}

void Watch::addDirWatch(string path, bool recursive){
    auto result = dirIndex.insert({path, recursive});
    if(result.second){ // check if insertion was successful i.e. result.second = true (false when already exists in map)
        cout << "Watch: "<< "Added watch to directory: " << path << endl;
        sqlQueue << "INSERT or IGNORE INTO dirIndex (PATH, RECURSIVE) VALUES ('" << path << "'," << (recursive ? "TRUE" : "FALSE") << ");"; // queue SQL insert
        for (const auto & entry : fs::directory_iterator(path)){ // iterate through all directory entries
            fs::file_status s = fs::status(entry);
            if(fs::is_directory(s) && recursive) { // check recursive flag before adding directories recursively
                //cout << "Recursively adding: " << entry.path() << endl;
                addWatch(entry.path().string(), true); 
            } else if(fs::is_regular_file(s)){
                addFileWatch(entry.path().string());
            } else if(!fs::is_directory(s)){ 
                cout << "Watch: " << "Unknown file encountered: " << entry.path().string() << endl; 
            }
        }
    } else { // duplicate - directory was not added
        cout << "Watch: " << "Watch to directory already exists: " << path << endl;
    }
}

void Watch::addFileWatch(string path){
    auto fstime = fs::last_write_time(path);
    std::time_t modtime = decltype(fstime)::clock::to_time_t(fstime); 
    auto result = fileIndex.insert({path, modtime});
    if(result.second){ // check if insertion was successful i.e. result.second = true (false when already exists in map)
        cout << "Watch: " << "Added watch to file: " << path << endl;
        sqlQueue << "INSERT or IGNORE INTO fileIndex (PATH, MODTIME) VALUES ('" << path << "'," << modtime << ");"; // if successful, queue an SQL insert into DB
        remote->queueForUpload(path, path); // queue for upload on remote 
    } else { // duplicate
        cout << "Watch: " << "Watch to file already exists: " << path << endl;
    }
}

void Watch::scanFileChange(){
    // existing files that are being watched
    for(auto elem: fileIndex){
        if(!fs::exists(elem.first)){ // if file has been deleted
            // code to update remotes as appropriate
            std::lock_guard<std::mutex> guard(mtx);
            cout << "Watch: " << "File no longer exists: " << elem.first << endl;
            fileIndex.erase(elem.first); // remove file from watch list
            sqlQueue << "DELETE from fileIndex where PATH='" << elem.first << "';"; // queue deletion from DB
            break;
        }
        auto recentfsTime = fs::last_write_time(elem.first);
        std::time_t recentModTime = decltype(recentfsTime)::clock::to_time_t(recentfsTime);
        if(recentModTime != elem.second){ // if current last_write_time of file != saved value, file has changed
            std::lock_guard<std::mutex> guard(mtx);
            cout << "Watch: " << "File change detected: " << elem.first << endl;
            fileIndex[elem.first] = recentModTime; // amend in fileIndex map
            sqlQueue << "UPDATE fileIndex SET MODTIME = '" << recentModTime << "' WHERE PATH='" << elem.first << "';"; // queue SQL update
        }
    }
    // check watched directories for new files and directories
    for(auto elem: dirIndex){
        if(!fs::exists(elem.first)){ // if directory has been deleted
            std::lock_guard<std::mutex> guard(mtx);
            cout << "Watch: " << "Directory no longer exists: " << elem.first << endl;
            dirIndex.erase(elem.first); // remove watch to directory
            sqlQueue << "DELETE from dirIndex where PATH='" << elem.first << "';"; // queue deletion from DB
            break;
        }
        for (const auto &entry : fs::directory_iterator(elem.first)){ // iterate through all directory entries
            fs::file_status s = fs::status(entry);
            if(fs::is_directory(s) && elem.second) {// check recursive flag (elem.second) is true before checking if watch to dir already exists
                if(!dirIndex.count(entry.path())){   // check if directory already exists in watched map
                    std::lock_guard<std::mutex> guard(mtx);
                    cout << "Watch: " << "New directory found: " << elem.first << endl;
                    addDirWatch(entry.path().string(), true);  // add new directory and any files contained within
                }
            } else if(fs::is_regular_file(s)){
                if(!fileIndex.count(entry.path())){  // check if each file already exists
                    std::lock_guard<std::mutex> guard(mtx);
                    cout << "Watch: " << "New file found: " << elem.first << endl;
                    addFileWatch(entry.path().string());
                }
            }
        }
    }
}

void Watch::execQueuedSQL(){
    std::lock_guard<std::mutex> guard(mtx);
    if(sqlQueue.rdbuf()->in_avail() != 0) { // if queue is not empty
        db->execSQL(sqlQueue.str().c_str()); // convert to c style string and execute bucket
        sqlQueue.str(""); // empty bucket
        sqlQueue.clear(); // clear error codes
    }
}

void Watch::displayWatchDirs(){
    cout << "\nWatched directories: " << endl;
    for(auto elem: dirIndex){
        cout << elem.first << " recursive: " << elem.second << endl;
    }
}

void Watch::displayWatchFiles(){
    cout << "\nWatched files: " << endl;
    for(auto elem: fileIndex){
        cout << elem.first << " modtime: " << displayTime(elem.second);
    }
}

string Watch::displayTime(std::time_t modtime) const{
    return std::asctime(std::localtime(&modtime));
}

void Watch::listDir(string path){
    for (const auto & entry : fs::directory_iterator(path)){
        cout << entry.path();
        fileAttributes(entry);
    }
}

void Watch::fileAttributes(const fs::path& path){
    fs::file_status s = fs::status(path);
    // alternative: switch(s.type()) { case fs::file_type::regular: ...}
    if(fs::is_regular_file(s)) std::cout << " is a regular file\n";
    if(fs::is_directory(s)) std::cout << " is a directory\n";
    if(fs::is_block_file(s)) std::cout << " is a block device\n";
    if(fs::is_character_file(s)) std::cout << " is a character device\n";
    if(fs::is_fifo(s)) std::cout << " is a named IPC pipe\n";
    if(fs::is_socket(s)) std::cout << " is a named IPC socket\n";
    if(fs::is_symlink(s)) std::cout << " is a symlink\n";
    if(!fs::exists(s)) std::cout << " does not exist\n";
    if(fs::is_empty(path)){ std::cout << "empty\n"; } else { std::cout << "not empty\n"; };

    // size
    try {
        std::cout << "size: " << fs::file_size(path) << endl; // attempt to get size of a file
    } catch(fs::filesystem_error& e) { // e.g. is a directory, no size
        std::cout << e.what() << '\n';
    }

    // last modification time
    //std::cout << "File write time is " << displayTime(fs::last_write_time(path)) << endl;
}
