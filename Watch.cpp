#include <./Watch.h>

Watch::Watch(string path){ // constructor
    this->path = path;
}

void Watch::listDir(){
    //std::string path = "/path/to/directory";
    for (const auto & entry : fs::directory_iterator(path)){
        std::cout << entry.path() << std::endl;
    }
}
