#include <enclone/remote/S3.h>

// example API calls - https://docs.aws.amazon.com/sdk-for-cpp/v1/developer-guide/examples-s3-objects.html

S3::S3(std::atomic_bool *runThreads)
{
    this->runThreads = runThreads;
    queue = new Queue();            

    // S3 logging options
    options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Info; // turn logging on using the default logger
    options.loggingOptions.defaultLogPrefix = "log/aws_sdk_";
}

S3::~S3()
{
    delete queue;
}

void S3::execThread(){
    while(*runThreads){
        cout << "S3: Calling S3 API..." << endl; cout.flush(); 
        callAPI();
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}

void S3::callAPI(){
    Aws::InitAPI(options);
    {
        Aws::S3::S3Client s3_client;
        //listBuckets(s3_client);
        listObjects(s3_client);
        uploadQueue(s3_client);
    }
    Aws::ShutdownAPI(options);
}

bool S3::queueForUpload(std::string path, std::string objectName){
    std::lock_guard<std::mutex> guard(mtx);
    return queue->pushUpload(path, objectName);
}

void S3::uploadQueue(Aws::S3::S3Client s3_client){
    std::lock_guard<std::mutex> guard(mtx);
    std::pair<string, string> returnValue;
    std::pair<string, string> *returnValuePtr = &returnValue;
    while(queue->popUpload(returnValuePtr)){ // returns true until queue is empty
        cout << "S3: Uploading" << returnValuePtr->first << " : " << returnValuePtr->second << endl;
    }
    cout << "S3: uploadQueue is empty" << endl;
}

bool S3::listBuckets(Aws::S3::S3Client s3_client){
    auto outcome = s3_client.ListBuckets();
    if (outcome.IsSuccess())
    {
        std::cout << "S3: List of available buckets:" << std::endl;

        Aws::Vector<Aws::S3::Model::Bucket> bucket_list = outcome.GetResult().GetBuckets();

        for (auto const &bucket : bucket_list)
        {
            std::cout << bucket.GetName() << std::endl;
        }

        std::cout << std::endl;
        return true;
    } else {
        std::cout << "S3: ListBuckets error: "
                  << outcome.GetError().GetExceptionName() << std::endl
                  << outcome.GetError().GetMessage() << std::endl;
        return false;
    }
}

bool S3::listObjects(Aws::S3::S3Client s3_client){
    Aws::S3::Model::ListObjectsRequest objects_request;
    objects_request.WithBucket(BUCKET_NAME);

    auto list_objects_outcome = s3_client.ListObjects(objects_request);

    if (list_objects_outcome.IsSuccess()) {
        Aws::Vector<Aws::S3::Model::Object> object_list =
            list_objects_outcome.GetResult().GetContents();
        std::cout << "S3: Files on S3 bucket " << BUCKET_NAME << std::endl;
        for (auto const &s3_object : object_list)
        {
            std::cout << "* " << s3_object.GetKey() << std::endl;
        }
        return true;
    } else {
        std::cout << "S3: ListObjects error: " <<
            list_objects_outcome.GetError().GetExceptionName() << " " <<
            list_objects_outcome.GetError().GetMessage() << std::endl;
        return false;
    }
}

bool S3::put_s3_object( Aws::S3::S3Client s3_client,
                        const Aws::String& s3_bucketName, 
                        const Aws::String& s3_objectName, 
                        const std::string& path)
{
    // check again that path exists
    if (!fs::exists(path)) {
        std::cout << "S3: ERROR: NoSuchFile: The specified file does not exist" << std::endl;
        return false;
    }

    Aws::S3::Model::PutObjectRequest object_request;

    object_request.SetBucket(s3_bucketName);
    object_request.SetKey(s3_objectName);
    const std::shared_ptr<Aws::IOStream> input_data = 
        Aws::MakeShared<Aws::FStream>("SampleAllocationTag", 
                                      path.c_str(), 
                                      std::ios_base::in | std::ios_base::binary);
    object_request.SetBody(input_data);

    // Put the object
    auto put_object_outcome = s3_client.PutObject(object_request);
    if (!put_object_outcome.IsSuccess()) {
        auto error = put_object_outcome.GetError();
        std::cout << "S3: ERROR: " << error.GetExceptionName() << ": " 
            << error.GetMessage() << std::endl;
        return false;
    }
    return true;
}