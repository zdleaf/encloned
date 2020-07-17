#ifndef QUEUE_H
#define QUEUE_H

#include <iostream>
#include <string>
#include <filesystem>
#include <deque>
#include <memory>
#include <mutex>

namespace fs = std::filesystem;
using std::string;
using std::cout;
using std::endl;

class Queue{
    private:
        std::deque<std::pair<string, string>> uploadQueue; // pair<string path, string objectName>
        std::deque<std::pair<string, string>> downloadQueue; // pair<string path, string objectName>
        std::deque<string> deleteQueue;
        // download queue

    public:
        Queue();

        bool getFrontUpload(std::pair<string, string>* returnValue);
        bool enqueueUpload(std::string path, std::string objectName);
        bool dequeueUpload();
        bool uploadEmpty();

        bool enqueueDownload(std::string path, std::string objectName);
        bool dequeueDownload(std::pair<string, string>* returnValue);
        bool downloadEmpty();

        bool enqueueDelete(std::string objectName);
        bool dequeueDelete(std::string* returnValue);
        bool deleteEmpty();

};

#endif