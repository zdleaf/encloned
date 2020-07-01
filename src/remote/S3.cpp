#include <enclone/remote/S3.h>

// example API calls - https://docs.aws.amazon.com/sdk-for-cpp/v1/developer-guide/examples-s3-objects.html

S3::S3()
{
    options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Info; // turn logging on using the default logger
    options.loggingOptions.defaultLogPrefix = "log/aws_sdk_";
}

S3::~S3()
{
    
}

void S3::callAPI()
{
    Aws::InitAPI(options);
    {
        Aws::S3::S3Client s3_client;
        listBuckets(s3_client);
    }
    Aws::ShutdownAPI(options);
}

bool S3::listBuckets(Aws::S3::S3Client s3_client){
    auto outcome = s3_client.ListBuckets();
    if (outcome.IsSuccess())
    {
        std::cout << "List of available buckets:" << std::endl;

        Aws::Vector<Aws::S3::Model::Bucket> bucket_list = outcome.GetResult().GetBuckets();

        for (auto const &bucket : bucket_list)
        {
            std::cout << bucket.GetName() << std::endl;
        }

        std::cout << std::endl;
        return true;
    } else {
        std::cout << "ListBuckets error: "
                  << outcome.GetError().GetExceptionName() << std::endl
                  << outcome.GetError().GetMessage() << std::endl;
        return false;
    }
}