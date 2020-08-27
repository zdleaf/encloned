#ifndef S3_H
#define S3_H

#include <string>
#include <filesystem>
#include <fstream>
#include <memory>
#include <chrono>

// concurrency/multi-threading
#include <thread>
#include <mutex>
#include <atomic>

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/Bucket.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/Object.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>

#include <aws/transfer/TransferManager.h>

#include <enclone/remote/Queue.h>
#include <enclone/remote/Remote.h>
#include <enclone/Encryption.h>

namespace fs = std::filesystem;
using std::string;
using std::cout;
using std::endl;

class Remote;
class encloned;

class S3: public Queue {
    private:
        int remoteID = 1; // each remote has a unique remoteID

        Remote *remote; // ptr to class instance of Remote that spawned this S3 instance
        encloned *daemon;
        
        std::vector<string> remoteObjects;
        std::unordered_map<string, string> remoteObjectMap; // also stores last mod time in readable string format

        Aws::SDKOptions options;
        const Aws::String BUCKET_NAME = "enclone";

        // concurrency/multi-threading
        std::mutex mtx;
        std::atomic_bool *runThreads; // ptr to flag indicating if execThread should loop or close down

        // S3 specific
        void uploadFromQueue(std::shared_ptr<Aws::Transfer::TransferManager> transferManager);
        string downloadFromQueue(std::shared_ptr<Aws::Transfer::TransferManager> transferManager);
        string deleteFromQueue(std::shared_ptr<Aws::S3::S3Client> s3_client);

        bool listBuckets(std::shared_ptr<Aws::S3::S3Client> s3_client);
        string listObjects(std::shared_ptr<Aws::S3::S3Client> s3_client);

        bool uploadObject(std::shared_ptr<Aws::Transfer::TransferManager> transferManager, const Aws::String& bucketName, const std::string& path, const std::string& objectName);
        string downloadObject(std::shared_ptr<Aws::Transfer::TransferManager> transferManager, const Aws::String& bucketName, const std::string& writeToPath, const std::string& objectName, std::time_t& originalModTime, std::string& targetPath); // download, restore modtime and verify hashes
        string downloadObject(std::shared_ptr<Aws::Transfer::TransferManager> transferManager, const Aws::String& bucketName, const std::string& writeToPath, const std::string& objectName); // do not verify, do not restore modtime - used for index backup only
        string deleteObject(std::shared_ptr<Aws::S3::S3Client> s3_client, const Aws::String& objectName, const Aws::String& fromBucket);
        
        //bool put_s3_object(const Aws::String& s3_bucket_name, const std::string& path, const Aws::String& s3_object_name);
        //bool get_s3_object(const Aws::String& objectName, const Aws::String& fromBucket);

    public:
        S3(std::atomic_bool *runThreads, Remote *remote);
        ~S3();

        // delete copy constructors - this class should not be copied
        S3(const S3&) = delete;
        S3& operator=(const S3&) = delete;

        void execThread();
        string callAPI(string arg);

        std::vector<string> getObjects();
        std::unordered_map<string, string> getObjectMap();

};

#endif