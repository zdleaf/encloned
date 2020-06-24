#include <enclone/Watch.h>

Watch::Watch(){ // constructor

}

void Watch::addWatch(string path, bool recursive){
    fs::file_status s = fs::status(path);
    if(fs::is_directory(s)){ // adding a directory to watch
        addDirWatch(path, recursive);
    } else if(fs::is_regular_file(s)){ // adding a regular file to watch
        addFileWatch(path);
    } else {
        std::cout << path << " is not a valid directory/file" << endl;
    }
}

void Watch::scanFileChange(){
    // existing files that are being watched
    for(auto elem: watchFiles){
        if(!fs::exists(elem.first)){ // if file has been deleted
            // code to update remotes as appropriate
            cout << "File no longer exists: " << elem.first << endl;
            watchFiles.erase(elem.first); // remove file from watch list
            break;
        }
        auto recentModTime = fs::last_write_time(elem.first);
        if(recentModTime != elem.second){ // if current last_write_time of file != saved value, file has changed
            cout << "File change detected: " << elem.first << endl;
            // code to handle new updated file
            watchFiles[elem.first] = recentModTime;
        }
    }
/*     for(auto dir: watchDirs){
        for (const auto &entry : fs::directory_iterator(dir)){ // iterate through all directory entries
            fs::file_status s = fs::status(entry);
            if(fs::is_directory(s) && recursive) { 
                // new directory and recursive flag on folder?
            } else if(fs::is_regular_file(s)){
                // new file?
            }
        }
    } */
}

void Watch::addDirWatch(string &path, bool recursive){
    auto result = watchDirs.insert({path, recursive});
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
    auto result = watchFiles.insert({path, fs::last_write_time(path)});
    if(result.second){ // check if insertion was successful i.e. result.second = true
        cout << "Added watch to file: " << path << endl;
    } else { // duplicate
        cout << "Watch to file already exists: " << path << endl;
    }
/*     File f;
    f.path = path;
    f.modtime = fs::last_write_time(path);
    f.size = fs::file_size(path);
    watchFiles.insert(f); */  
}

void Watch::displayWatchDirs(){
    cout << "\nWatched directories: " << endl;
    for(auto elem: watchDirs){
        cout << elem.first << " recursive: " << elem.second << endl;
    }
}

void Watch::displayWatchFiles(){
    cout << "\nWatched files: " << endl;
    for(auto elem: watchFiles){
        cout << elem.first << " modtime: " << displayTime(elem.second);
    }
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
    std::cout << "File write time is " << displayTime(fs::last_write_time(path)) << endl;
}

string Watch::displayTime(fs::file_time_type modtime) const{
    std::time_t cftime = decltype(modtime)::clock::to_time_t(modtime);
    return std::asctime(std::localtime(&cftime));
}
