#include <enclone/remote/Queue.h>

Queue::Queue(){
    
}

bool Queue::enqueueUpload(std::string path, std::string objectName){
    std::pair<string, string> item;
    if (!fs::exists(path)) {
        std::cout << "Queue: Error: file does not exist - unable to add " << path << std::endl;
        return false;
    }
    // check if object already exists on remote
    item = std::make_pair(path, objectName);
    uploadQueue.push_back(item);
    return true;
}

bool Queue::dequeueUpload(std::pair<string, string> *returnValue){
    if(uploadQueue.empty()){ 
        return false; 
    } else if(!uploadQueue.empty()){
        *returnValue = uploadQueue.front();
        uploadQueue.pop_front(); // delete element once we've returned
    }
    return true;
}

bool Queue::enqueueDelete(std::string objectName){
    deleteQueue.push_back(objectName);
    return true;
}

bool Queue::dequeueDelete(std::string* returnValue){
    if(deleteQueue.empty()){ 
        return false; 
    } else if(!deleteQueue.empty()){
        *returnValue = deleteQueue.front();
        deleteQueue.pop_front(); // delete element once we've returned
    }
    return true;
}

bool Queue::uploadEmpty(){
    return uploadQueue.empty();
}

bool Queue::deleteEmpty(){
    return deleteQueue.empty();
}