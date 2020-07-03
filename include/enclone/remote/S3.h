#ifndef S3_H
#define S3_H

#include <string>
#include <filesystem>
#include <fstream>
#include <memory>

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

#include <enclone/remote/Queue.h>

namespace fs = std::filesystem;
using std::string;
using std::cout;
using std::endl;

class S3 {
    private:
        std::shared_ptr<Queue> queue; // queue of items to be uploaded

        Aws::SDKOptions options;
        const Aws::String BUCKET_NAME = "enclone";
        
        // concurrency/multi-threading
        std::mutex mtx;
        std::atomic_bool *runThreads; // ptr to flag indicating if execThread should loop or close down

        // S3 specific
        bool listBuckets(Aws::S3::S3Client s3_client);
        bool listObjects(Aws::S3::S3Client s3_client);
        bool put_s3_object(Aws::S3::S3Client s3_client, const Aws::String& s3_bucket_name, const Aws::String& s3_object_name, const std::string& path);

    public:
        S3(std::atomic_bool *runThreads);
        ~S3();

        // delete copy constructors - this class should not be copied
        S3(const S3&) = delete;
        S3& operator=(const S3&) = delete;

        void callAPI();
        void execThread();

        // generic
        bool queueForUpload(std::string path, std::string objectName);
        bool queueForDelete();
        void uploadQueue(Aws::S3::S3Client s3_client);

};

#endif