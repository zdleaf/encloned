#include <enclone/remote/S3.h>

// example API calls - https://docs.aws.amazon.com/sdk-for-cpp/v1/developer-guide/examples-s3-objects.html

S3::S3(std::atomic_bool *runThreads, Remote *remote)
{
    this->remote = remote;
    this->runThreads = runThreads;       

    // S3 logging options
    options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Info; // turn logging on using the default logger
    options.loggingOptions.defaultLogPrefix = "log/aws_sdk_";
}

S3::~S3()
{

}

void S3::execThread(){
    while(*runThreads){
        cout << "S3: Calling S3 API..." << endl; cout.flush();
        //callAPI("transfer");
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}

string S3::callAPI(string arg){
    if(arg == "upload" && uploadQueue.empty()) { // no need to connect to API if there is nothing to upload/download
        //cout << "S3: uploadQueue and downloadQueue are empty" << endl;
        return "";
    }
    if(arg == "download" && downloadQueue.empty()) { // no need to connect to API if there is nothing to upload/download
        //cout << "S3: uploadQueue and downloadQueue are empty" << endl;
        return "";
    }
    string response;
    Aws::InitAPI(options);
    {
        std::shared_ptr<Aws::S3::S3Client> s3_client = Aws::MakeShared<Aws::S3::S3Client>("S3Client");
        auto executor = Aws::MakeShared<Aws::Utils::Threading::PooledThreadExecutor>("executor", 25);
        Aws::Transfer::TransferManagerConfiguration transferConfig(executor.get());
        transferConfig.s3Client = s3_client;
        auto transferManager = Aws::Transfer::TransferManager::Create(transferConfig);

        if(arg == "upload"){
            uploadFromQueue(transferManager);
        } else if (arg == "download"){
            response = downloadFromQueue(transferManager);
        } else if (arg == "listObjects"){
            try {
                response = listObjects(s3_client);
            } catch(const std::exception& e){ // listObjects may fail due invalid credentials etc
                throw;
            }
        } else if (arg == "delete"){
            deleteQueue();
        } else if (arg == "listBuckets"){
            response = listBuckets(s3_client);
        } 
    }
    Aws::ShutdownAPI(options);
    return response;
}

void S3::uploadFromQueue(std::shared_ptr<Aws::Transfer::TransferManager> transferManager){
    std::lock_guard<std::mutex> guard(mtx);
    if(uploadQueue.empty()){ return; } 
    for(auto item: uploadQueue){ 
        auto [path, pathHash, modtime] = item; // get values out of the tuple
        
        // check file still exists
        if(!fs::exists(path)){
            std::stringstream error;
            error << "S3: Error: File " << path << " no longer exists" << endl;
            cout << error.str();
            dequeueUpload(); // remove from queue
            continue; // go to the next item
        }

        // check that modtime for path is still valid - file may have changed since added to uploadQueue, or no longer exist
        try {
            auto fstime = fs::last_write_time(path); // get modtime from file
            std::time_t currentModtime = decltype(fstime)::clock::to_time_t(fstime);
            if(modtime != currentModtime){
                std::stringstream error;
                error << "S3: Error: File " << path << " has changed - unable to upload version with hash " << pathHash << endl;
                cout << error.str();
                dequeueUpload(); // remove from queue
                continue; // go to the next item
            }
        } catch (const std::exception& ex){
            cout << ex.what() << endl;
        }

        try {
            uploadObject(transferManager, BUCKET_NAME, path, pathHash);
        } catch(const std::exception& e){ // listObjects may fail due invalid credentials etc
            continue; // go to the next item, but do not remove failed item from queue
        }
        dequeueUpload(); // if success, pop the object from the front of the queue
    }
    //cout << "S3: uploadQueue is empty" << endl; cout.flush();
}

string S3::downloadFromQueue(std::shared_ptr<Aws::Transfer::TransferManager> transferManager){
    std::ostringstream ss;
    std::lock_guard<std::mutex> guard(mtx);
    if(!downloadQueue.empty()){ 
        std::pair<string, string> returnValue;
        std::pair<string, string> *returnValuePtr = &returnValue;
        while(dequeueDownload(returnValuePtr)){ // returns true until queue is empty
            ss << downloadObject(transferManager, BUCKET_NAME, returnValuePtr->first, returnValuePtr->second);
        }
        cout << "S3: downloadFromQueue complete" << endl; cout.flush();
    } else { ss << "S3: downloadQueue is empty" << endl; cout.flush(); }
    return ss.str();
}

void S3::deleteQueue(){
    std::lock_guard<std::mutex> guard(mtx);
    std::string returnValue;
    std::string* returnValuePtr = &returnValue;
    while(dequeueDelete(returnValuePtr)){ // returns true until queue is empty
        Aws::String objectName(*returnValuePtr);
        //bool result = deleteObject(objectName, BUCKET_NAME);
    }
    cout << "S3: deleteQueue is empty" << endl;
}

bool S3::listBuckets(std::shared_ptr<Aws::S3::S3Client> s3_client){
    auto outcome = s3_client->ListBuckets();
    if (outcome.IsSuccess())
    {
        std::cout << "S3: List of available buckets:" << std::endl;

        Aws::Vector<Aws::S3::Model::Bucket> bucket_list = outcome.GetResult().GetBuckets();

        for (auto const &bucket : bucket_list)
        {
            std::cout << bucket.GetName() << std::endl;
        }
        return true;
    } else {
        std::cout << "S3: listBuckets error: "
                  << outcome.GetError().GetExceptionName() << std::endl
                  << outcome.GetError().GetMessage() << std::endl;
        return false;
    }
}

string S3::listObjects(std::shared_ptr<Aws::S3::S3Client> s3_client){
    std::ostringstream response;
    Aws::S3::Model::ListObjectsRequest objects_request;
    objects_request.WithBucket(BUCKET_NAME);

    auto list_objects_outcome = s3_client->ListObjects(objects_request);

    if (list_objects_outcome.IsSuccess()) {
        remoteObjects.clear(); // erase old remoteObjects vector
        Aws::Vector<Aws::S3::Model::Object> object_list =
            list_objects_outcome.GetResult().GetContents();
        if(object_list.empty()){
            response << "S3: bucket " << BUCKET_NAME << " is empty" << std::endl;
        } else {
            response << "S3: Files on S3 bucket " << BUCKET_NAME << ":" << std::endl;
            for (auto const &s3_object : object_list)
            {
                auto modtime = s3_object.GetLastModified().ToGmtString(Aws::Utils::DateFormat::ISO_8601);
                response << s3_object.GetKey() << " : " << modtime << std::endl;
                remoteObjects.push_back(s3_object.GetKey().c_str());
            }
        }
    } else {
        response << "S3: listObjects error: " <<
            list_objects_outcome.GetError().GetExceptionName() << " " <<
            list_objects_outcome.GetError().GetMessage() << std::endl;
        throw std::runtime_error("S3: listObjects error - unable to get objects from S3, check ~/.aws/credentials are valid\n");
    }
    cout << response.str();
    return response.str();
}

std::vector<string> S3::getObjects(){
    try {
        callAPI("listObjects"); // populate remoteObjects vector by calling listObjects via API
    } catch(const std::exception& e){ // listObjects may fail due invalid credentials etc
        throw;
    }
    return remoteObjects;
}

bool S3::uploadObject(std::shared_ptr<Aws::Transfer::TransferManager> transferManager, const Aws::String& bucketName, const std::string& path, const std::string& objectName){
    Aws::String awsPath(path);
    Aws::String awsObjectName(objectName);

    auto uploadHandle = transferManager->UploadFile(awsPath, bucketName, awsObjectName, "", Aws::Map<Aws::String, Aws::String>());
    uploadHandle->WaitUntilFinished();
    if(uploadHandle->GetStatus() == Aws::Transfer::TransferStatus::COMPLETED){
        cout << "S3: Upload of " << path << " as " << objectName << " successful" << endl;
        assert(uploadHandle->GetBytesTotalSize() == uploadHandle->GetBytesTransferred()); // verify upload expected length of data
        remote->uploadSuccess(path, objectName, remoteID);
        return true;
    } else {
        std::ostringstream error;
        error << "S3: Upload of " << path << " as " << objectName << " failed with status: " << uploadHandle->GetStatus() << "(" << uploadHandle->GetLastError().GetMessage() << ")" << endl;
        cout << error.str();
        throw std::runtime_error(error.str());
    }
    return false;
}

string S3::downloadObject(std::shared_ptr<Aws::Transfer::TransferManager> transferManager, const Aws::String& bucketName, const std::string& writeToPath, const std::string& objectName){
    std::ostringstream ss;
    // the directory we want to download to
    string downloadDir = "/home/zach/enclone/dl/"; 

    // split path into directory path and filename
    std::size_t found = writeToPath.find_last_of("/");
    string dirPath = downloadDir + writeToPath.substr(0,found+1); // put 
    string fileName = writeToPath.substr(found+1);
    cout << "path is " << dirPath << ", fileName is " << fileName << endl;

    // need to create the full directory structure at path if it's doesn't already exist, or download will say successful but not save to disk
    try {
        fs::create_directories(dirPath);
        cout << "Created directories for path: " << dirPath << endl;
    }
    catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }

    Aws::String awsWriteToFile(dirPath + fileName);
    Aws::String awsObjectName(objectName);

    auto downloadHandle = transferManager->DownloadFile(bucketName, awsObjectName, awsWriteToFile);
    downloadHandle->WaitUntilFinished();
    if(downloadHandle->GetStatus() == Aws::Transfer::TransferStatus::COMPLETED){
        if (downloadHandle->GetBytesTotalSize() == downloadHandle->GetBytesTransferred()) {
            ss << "S3: Download of " << objectName << " to " << awsWriteToFile << " successful" << endl;
            cout << ss.str();
            return ss.str();
        } else {
            ss << "S3: Bytes downloaded did not equal requested number of bytes: " << downloadHandle->GetBytesTotalSize() << downloadHandle->GetBytesTransferred() << std::endl;
        }
    } else {
        ss << "S3: Download of " << objectName << " to " << awsWriteToFile << " failed with message: " << downloadHandle->GetStatus() << endl;
    }

    size_t retries = 0; // retry download if failed (up to 5 times)
    while (downloadHandle->GetStatus() == Aws::Transfer::TransferStatus::FAILED && retries++ < 5)
    {
        std::cout << "S3: Retrying download again. Attempt " << retries << " of 5" << std::endl;
        transferManager->RetryDownload(downloadHandle);
        downloadHandle->WaitUntilFinished();
    }

    ss << "S3: Download of " << objectName << " to " << awsWriteToFile << " failed after maximum retry attempts" << std::endl;
    cout << ss.str();
    return ss.str();
}

// legacy upload/delete/download below - not using TransferManager

/* bool S3::put_s3_object( const Aws::String& s3_bucketName, 
                        const std::string& path,
                        const Aws::String& s3_objectName)
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
    auto put_object_outcome = s3_client->PutObject(object_request);
    if (!put_object_outcome.IsSuccess()) {
        auto error = put_object_outcome.GetError();
        std::cout << "S3: ERROR: " << error.GetExceptionName() << ": " 
            << error.GetMessage() << std::endl;
        cout << "S3: Upload of " << path << " as " << s3_objectName << " failed" << endl;
        return false;
    }
    cout << "S3: Upload of " << path << " as " << s3_objectName << " successful" << endl; 
    return true;
}

bool S3::deleteObject(const Aws::String& objectName, const Aws::String& fromBucket){
    Aws::S3::Model::DeleteObjectRequest request;

    request.WithKey(objectName).WithBucket(fromBucket);

    Aws::S3::Model::DeleteObjectOutcome result = s3_client->DeleteObject(request);

    if (!result.IsSuccess())
    {
        cout << "S3: Delete of " << objectName << " failed" << endl;
        auto err = result.GetError();
        std::cout << "Error: DeleteObject: " <<
            err.GetExceptionName() << ": " << err.GetMessage() << std::endl;
        return false;
    }
    cout << "S3: Delete of " << objectName << " successful" << endl;
    return true;
}

bool S3::get_s3_object(const Aws::String& objectName, const Aws::String& fromBucket)
{
    // s3_client.getObject(new GetObjectRequest(bucket,key),file)

    Aws::S3::Model::GetObjectRequest objectRequest;
    objectRequest.SetBucket(fromBucket);
    objectRequest.SetKey(objectName);

    Aws::S3::Model::GetObjectOutcome getObjectOutcome = s3_client->GetObject(objectRequest);

    if (getObjectOutcome.IsSuccess())
    {
        auto& retrieved_file = getObjectOutcome.GetResultWithOwnership().GetBody();

        // Print a beginning portion of the text file.
        std::cout << "Beginning of file contents:\n";
        char file_data[255] = { 0 };
        retrieved_file.getline(file_data, 254);
        std::cout << file_data << std::endl;

        return true;
    }
    else
    {
        auto err = getObjectOutcome.GetError();
        std::cout << "Error: GetObject: " <<
            err.GetExceptionName() << ": " << err.GetMessage() << std::endl;

        return false;
    }
} */