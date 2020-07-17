#include <enclone/remote/Queue.h>

Queue::Queue(){
    uploadQueue = std::make_shared<std::deque<std::tuple<string, string, std::time_t>>>();
    downloadQueue = std::make_shared<std::deque<std::pair<string, string>>>();
}

bool Queue::enqueueUpload(std::string path, std::string objectName, std::time_t modtime){
    std::tuple<string, string, std::time_t> item;
    if (!fs::exists(path)) {
        std::cout << "Queue: Error: file does not exist - unable to add " << path << std::endl;
        return false;
    }
    // check if object already exists on remote
    item = std::make_tuple(path, objectName, modtime);
    uploadQueue->push_back(item);
    return true;
}

bool Queue::dequeueUpload(){
    if(uploadQueue->empty()){ 
        return false; 
    } else if(!uploadQueue->empty()){
        uploadQueue->pop_front(); // delete element once we've returned
    }
    return true;
}

bool Queue::enqueueDownload(std::string path, std::string objectName){
    std::pair<string, string> item;
    item = std::make_pair(path, objectName);
    downloadQueue->push_back(item);
    return true;
}

bool Queue::dequeueDownload(std::pair<string, string> *returnValue){
    if(downloadQueue->empty()){ 
        return false; 
    } else if(!downloadQueue->empty()){
        *returnValue = downloadQueue->front();
        downloadQueue->pop_front(); // delete element once we've returned
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