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
        std::deque<string> deleteQueue;

    public:
        Queue();

        bool enqueueUpload(std::string path, std::string objectName);
        bool dequeueUpload(std::pair<string, string>* returnValue);
        bool uploadEmpty();

        bool enqueueDelete();
        void dequeueDelete();
        bool deleteEmpty();

};

#endif