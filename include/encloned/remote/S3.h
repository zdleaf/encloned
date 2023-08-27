#ifndef S3_H
#define S3_H

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

// concurrency/multi-threading
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/Bucket.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/Object.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/transfer/TransferManager.h>
#include <encloned/Encryption.h>
#include <encloned/remote/Queue.h>
#include <encloned/remote/Remote.h>

#include <atomic>
#include <mutex>
#include <thread>

namespace fs = std::filesystem;
using std::cout;
using std::endl;
using std::string;

class Remote;
class encloned;

class S3 : public Queue {
 private:
  int remoteID = 1;  // each remote has a unique remoteID

  // ptr to class instance of Remote that spawned this S3 instance
  Remote* remote;
  encloned* daemon;

  std::vector<string> remoteObjects;
  // also stores last mod time in readable string format
  std::unordered_map<string, string> remoteObjectMap;

  Aws::SDKOptions options;
  const Aws::String BUCKET_NAME = "enclone";

  // concurrency/multi-threading
  std::mutex mtx;
  std::atomic_bool* runThreads;  // ptr to flag indicating if execThread should
                                 // loop or close down

  // S3 specific
  void uploadFromQueue(
      std::shared_ptr<Aws::Transfer::TransferManager> transferManager);
  string downloadFromQueue(
      std::shared_ptr<Aws::Transfer::TransferManager> transferManager);
  string deleteFromQueue(std::shared_ptr<Aws::S3::S3Client> s3_client);

  bool listBuckets(std::shared_ptr<Aws::S3::S3Client> s3_client);
  string listObjects(std::shared_ptr<Aws::S3::S3Client> s3_client);

  bool uploadObject(
      std::shared_ptr<Aws::Transfer::TransferManager> transferManager,
      const Aws::String& bucketName, const std::string& path,
      const std::string& objectName);
  // download, restore modtime and verify hashes
  string downloadObject(
      std::shared_ptr<Aws::Transfer::TransferManager> transferManager,
      const Aws::String& bucketName, const std::string& writeToPath,
      const std::string& objectName, std::time_t& originalModTime,
      std::string& targetPath);
  // do not verify, do not restore modtime - used for index backup only
  string downloadObject(
      std::shared_ptr<Aws::Transfer::TransferManager> transferManager,
      const Aws::String& bucketName, const std::string& writeToPath,
      const std::string& objectName);
  string deleteObject(std::shared_ptr<Aws::S3::S3Client> s3_client,
                      const Aws::String& objectName,
                      const Aws::String& fromBucket);

  int encryptionQueueTime;
  int decryptionQueueTime;
  // bool put_s3_object(const Aws::String& s3_bucket_name, const std::string&
  // path, const Aws::String& s3_object_name); bool get_s3_object(const
  // Aws::String& objectName, const Aws::String& fromBucket);

 public:
  S3(std::atomic_bool* runThreads, Remote* remote);
  ~S3();

  S3(const S3&) = delete;
  S3& operator=(const S3&) = delete;

  void execThread();
  string callAPI(string arg);

  std::vector<string> getObjects();
  std::unordered_map<string, string> getObjectMap();
};

#endif
