#include <enclone/Watch.h>

Watch::Watch(){ // constructor

}

void Watch::addWatch(string path, bool recursive){
    fs::file_status s = fs::status(path);
    if(fs::is_directory(s)){ // adding a directory to watch
        watchDirs.push_back(path);
        cout << "Added watch to directory: " << path << endl;
        if(recursive){ // recursively add subdirs
            for (const auto & entry : fs::directory_iterator(path)){ // iterate through all directory entries
                fs::file_status s = fs::status(entry);
                if(fs::is_directory(s)) { 
                        cout << "Recursively adding: " << entry.path() << endl;
                        addWatch(entry.path().string(), true); 
                    }
            }
        }
    } else if(fs::is_regular_file(s)){ // adding a regular file to watch
        watchFiles.push_back(path);
        cout << "Added watch to file: " << path << endl;
    } else {
        std::cout << path << " is not a valid directory/file" << endl;
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
    auto ftime = fs::last_write_time(path);
    std::time_t cftime = decltype(ftime)::clock::to_time_t(ftime);
    std::cout << "File write time is " << std::asctime(std::localtime(&cftime)) << '\n';
    
    cout << "\n";
}
