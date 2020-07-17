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
    protected:
        std::deque<std::tuple<string, string, std::time_t>> uploadQueue; // pair<string path, string objectName>
        std::deque<std::pair<string, string>> downloadQueue; // pair<string path, string objectName>
        std::deque<string> deleteQueue;
        // download queue

    public:
        Queue();

        bool enqueueUpload(std::string path, std::string objectName, std::time_t modtime);
        bool dequeueUpload();

        bool enqueueDownload(std::string path, std::string objectName);
        bool dequeueDownload(std::pair<string, string>* returnValue);

        bool enqueueDelete(std::string objectName);
        bool dequeueDelete(std::string* returnValue);

};

#endif