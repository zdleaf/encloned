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
        std::deque<std::tuple<string, string, std::time_t, string>> downloadQueue; // tuple<path, objectName, modtime, targetPath>
        std::deque<string> deleteQueue; // objectName
        // download queue

    public:
        Queue();

        bool enqueueUpload(std::string path, std::string objectName, std::time_t modtime);
        bool dequeueUpload();

        bool enqueueDownload(std::string path, std::string objectName, std::time_t modtime, string targetPath);
        bool dequeueDownload();

        bool enqueueDelete(std::string objectName);
        bool dequeueDelete();

};

#endif