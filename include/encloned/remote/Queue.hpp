#ifndef QUEUE_H
#define QUEUE_H

#include <deque>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>

namespace fs = std::filesystem;
using std::cout;
using std::endl;
using std::string;

class Queue {
 protected:
  std::deque<std::tuple<string, string, std::time_t>>
      uploadQueue;  // tuple<string path, objectName, modtime>
  std::deque<std::tuple<string, string, std::time_t, string>>
      downloadQueue;  // tuple<path, objectName, modtime, targetPath>
  std::deque<string> deleteQueue;  // objectName

 public:
  Queue();

  bool enqueueUpload(std::string path, std::string objectName,
                     std::time_t modtime);
  bool dequeueUpload();

  bool enqueueDownload(std::string path, std::string objectName,
                       std::time_t modtime, string targetPath);
  bool dequeueDownload();

  bool enqueueDelete(std::string objectName);
  bool dequeueDelete();
};

#endif
