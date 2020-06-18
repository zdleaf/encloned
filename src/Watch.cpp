#include <enclone/Watch.h>

Watch::Watch(string path){ // constructor
    this->path = path;
}

void Watch::listDir(){
    //std::string path = "/path/to/directory";
    for (const auto & entry : fs::directory_iterator(path)){
        std::cout << entry.path() << " isdir: " << fs::is_directory(entry) << " is_reg_file: " << fs::is_regular_file(entry) << " empty: " << fs::is_empty(entry) << " symlink: " << fs::is_symlink(entry) << endl;
    }
}
