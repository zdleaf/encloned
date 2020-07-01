#ifndef S3_H
#define S3_H

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/Bucket.h>

class S3 {
    private:
        Aws::SDKOptions options;

    public:
        S3();
        ~S3();

        void callAPI();
        bool listBuckets(Aws::S3::S3Client s3_client);

        void execThread();

        // delete copy constructors - this class should not be copied
        S3(const S3&) = delete;
        S3& operator=(const S3&) = delete;
};

#endif