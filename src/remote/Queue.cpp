#include <enclone/remote/Queue.h>

Queue::Queue(){
    
}

bool Queue::pushUpload(std::string path, std::string objectName){
    std::pair<string, string> item;
    if (!fs::exists(path)) {
        std::cout << "ERROR: The specified file does not exist - unable to add to upload queue" << std::endl;
        return false;
    }
    // check if object already exists on remote
    std::make_pair(path, objectName);
    uploadQueue.push_back(item);
    return true;
}

bool Queue::popUpload(std::pair<string, string> *returnValue){
    if(uploadQueue.empty()){ 
        return false; 
    } else if(!uploadQueue.empty()){
        *returnValue = uploadQueue.front();
        uploadQueue.pop_front(); // delete element once we've returned
    }
    return true;
}

bool Queue::uploadEmpty(){
    return uploadQueue.empty();
}

bool Queue::deleteEmpty(){
    return deleteQueue.empty();
}