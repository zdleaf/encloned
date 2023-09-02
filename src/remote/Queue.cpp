#include <encloned/remote/Queue.hpp>

Queue::Queue() {}

bool Queue::enqueueUpload(std::string path, std::string objectName,
                          std::time_t modtime) {
  std::tuple<string, string, std::time_t> item;
  if (!fs::exists(path)) {
    std::cout << "Queue: Error: file does not exist - unable to add " << path
              << std::endl;
    return false;
  }
  // check if object already exists on remote
  item = std::make_tuple(path, objectName, modtime);
  uploadQueue.push_back(item);
  return true;
}

bool Queue::dequeueUpload() {
  if (uploadQueue.empty()) {
    return false;
  } else if (!uploadQueue.empty()) {
    uploadQueue.pop_front();  // delete element once we've returned
  }
  return true;
}

bool Queue::enqueueDownload(std::string path, std::string objectName,
                            std::time_t modtime, string targetPath) {
  std::tuple<string, string, std::time_t, string> item;
  item = std::make_tuple(path, objectName, modtime, targetPath);
  downloadQueue.push_back(item);
  return true;
}

bool Queue::dequeueDownload() {
  if (downloadQueue.empty()) {
    return false;
  } else if (!downloadQueue.empty()) {
    downloadQueue.pop_front();  // delete element once we've returned
  }
  return true;
}

bool Queue::enqueueDelete(std::string objectName) {
  deleteQueue.push_back(objectName);
  return true;
}

bool Queue::dequeueDelete() {
  if (deleteQueue.empty()) {
    return false;
  } else if (!deleteQueue.empty()) {
    deleteQueue.pop_front();  // delete element once we've returned
  }
  return true;
}
