#include <enclone/Watch.h>

Watch::Watch(DB *db){ // constructor
    this->db = db; // set DB handle
}

Watch::~Watch(){ // destructor
    // delete db; // is this necessary?
}

void Watch::addWatch(string path, bool recursive){
    fs::file_status s = fs::status(path);
    if(!fs::exists(s)){                 // file/directory does not exist
        std::cout << path << " does not exist" << endl;
    } else if(fs::is_directory(s)){     // adding a directory to watch
        addDirWatch(path, recursive);
    } else if(fs::is_regular_file(s)){  // adding a regular file to watch
        addFileWatch(path);
    } else {                            // any other file type, e.g. IPC pipe
        std::cout << path << " is not a valid directory/file" << endl;
    }
}

void Watch::addDirWatch(string path, bool recursive){
    auto result = dirIndex.insert({path, recursive});
    if(result.second){ // check if insertion was successful i.e. result.second = true
        cout << "Added watch to directory: " << path << endl;

        for (const auto & entry : fs::directory_iterator(path)){ // iterate through all directory entries
            fs::file_status s = fs::status(entry);
            if(fs::is_directory(s) && recursive) { 
                //cout << "Recursively adding: " << entry.path() << endl;
                addWatch(entry.path().string(), true); 
            } else if(fs::is_regular_file(s)){
                addFileWatch(entry.path().string());
            } else if(!fs::is_directory(s)){ 
                cout << "Unknown file encountered: " << entry.path().string() << endl; 
            }
        }
    } else { // duplicate - directory was not added
        cout << "Watch to directory already exists: " << path << endl;
    }
}

void Watch::addFileWatch(string path){
    auto fstime = fs::last_write_time(path);
    std::time_t modtime = decltype(fstime)::clock::to_time_t(fstime); 
    auto result = fileIndex.insert({path, modtime});
    if(result.second){ // check if insertion was successful i.e. result.second = true
        cout << "Added watch to file: " << path << endl;
    } else { // duplicate
        cout << "Watch to file already exists: " << path << endl;
    }
}

void Watch::scanFileChange(){
    // existing files that are being watched
    for(auto elem: fileIndex){
        if(!fs::exists(elem.first)){ // if file has been deleted
            // code to update remotes as appropriate
            cout << "File no longer exists: " << elem.first << endl;
            fileIndex.erase(elem.first); // remove file from watch list
            break;
        }
        auto recentfsTime = fs::last_write_time(elem.first);
        std::time_t recentModTime = decltype(recentfsTime)::clock::to_time_t(recentfsTime);
        if(recentModTime != elem.second){ // if current last_write_time of file != saved value, file has changed
            cout << "File change detected: " << elem.first << endl;
            // code to handle new updated file
            fileIndex[elem.first] = recentModTime;
        }
    }
    // check watched directories for new files and directories
    for(auto elem: dirIndex){
        if(!fs::exists(elem.first)){ // if directory has been deleted
            cout << "Directory no longer exists: " << elem.first << endl;
            dirIndex.erase(elem.first); // remove watch to directory
            break;
        }
        for (const auto &entry : fs::directory_iterator(elem.first)){ // iterate through all directory entries
            fs::file_status s = fs::status(entry);
            if(fs::is_directory(s) && elem.second) {// check recursive flag (elem.second) is true before checking if watch to dir already exists
                if(!dirIndex.count(entry.path())){   // check if directory already exists in watched map
                    cout << "New directory found: " << elem.first << endl;
                    addDirWatch(entry.path().string(), true);  // add new directory and any files contained within
                }
            } else if(fs::is_regular_file(s)){
                if(!fileIndex.count(entry.path())){  // check if each file already exists
                    cout << "New file found: " << elem.first << endl;
                    addFileWatch(entry.path().string());
                }
            }
        }
    }
}

void Watch::indexToDB(){
    dirIndexToDB();
    fileIndexToDB();
    db->execSQL(sql.str().c_str()); // convert to c style string and execute bucket
    sql.clear(); // empty bucket
}

void Watch::dirIndexToDB(){
    for(auto elem: dirIndex){
        sql << "INSERT or IGNORE INTO dirIndex (PATH, RECURSIVE) VALUES ('" << elem.first << "'," << (elem.second ? "TRUE" : "FALSE") << ");";
    }
}

void Watch::fileIndexToDB(){
    for(auto elem: fileIndex){
        sql << "INSERT or IGNORE INTO fileIndex (PATH, MODTIME) VALUES ('" << elem.first << "'," << elem.second << ");";
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
