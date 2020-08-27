#include <enclone/remote/S3.h>

// example API calls - https://docs.aws.amazon.com/sdk-for-cpp/v1/developer-guide/examples-s3-objects.html

S3::S3(std::atomic_bool *runThreads, Remote *remote)
{
    this->remote = remote;
    this->runThreads = runThreads;       
    this->daemon = remote->getDaemon();
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
        //cout << "S3: uploadQueue is empty" << endl;
        return "";
    }
    if(arg == "download" && downloadQueue.empty()) { // no need to connect to API if there is nothing to upload/download
        //cout << "S3: downloadQueue is empty" << endl;
        return "";
    }
    if(arg == "delete" && deleteQueue.empty()) { // no need to connect to API if there is nothing to delete
        //cout << "S3: deleteQueue is empty" << endl;
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
        } else if (arg.substr(0, 9) == "uploadNow"){
            // split the arguments (path|pathHash)
            auto delimPos = arg.find('|', 10); // find second delimiter
            string path = std::string(&arg[10], &arg[delimPos]);
            string pathHash = arg.substr(delimPos+1);
            //cout << "DEBUG: path: " << path << " pathHash: " << pathHash << endl;
            try {
                uploadObject(transferManager, BUCKET_NAME, path, pathHash);
            } catch(const std::exception& e){ // listObjects may fail due invalid credentials etc
                response == e.what();
            }
        } else if (arg == "download"){
            response = downloadFromQueue(transferManager);
        } else if (arg.substr(0, 11) == "downloadNow"){
            // split the arguments (pathHash|target)
            auto delimPos = arg.find('|', 12); // find second delimiter
            string pathHash = std::string(&arg[12], &arg[delimPos]);
            string target = arg.substr(delimPos+1);
            //cout << "DEBUG: pathHash: " << pathHash << " target: " << target << endl;
            try {
                response = downloadObject(transferManager, BUCKET_NAME, target, pathHash);
            } catch(const std::exception& e){ // may fail due invalid credentials etc
                response = e.what();
            }
            //downloadObject(transferManager, BUCKET_NAME, target, pathHash, modtime, "./"); // we do not store modtime or a hash for the index database - so cannot verify this
            // need a separate downloadObject method that does not verify hash
        } else if (arg == "listObjects"){
            try {
                response = listObjects(s3_client);
            } catch(const std::exception& e){ // listObjects may fail due invalid credentials etc
                throw;
            }
        } else if (arg == "delete"){
            response = deleteFromQueue(s3_client);
        } else if (arg == "listBuckets"){
            response = listBuckets(s3_client);
        } 
    }
    Aws::ShutdownAPI(options);
    return response;
}

void S3::uploadFromQueue(std::shared_ptr<Aws::Transfer::TransferManager> transferManager){
    std::scoped_lock<std::mutex> guard(mtx);
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
    std::scoped_lock<std::mutex> guard(mtx);
    if(!downloadQueue.empty()){ 
        for(auto item: downloadQueue){
            auto [path, objectName, modtime, targetPath] = item;
            try {
                ss << downloadObject(transferManager, BUCKET_NAME, path, objectName, modtime, targetPath);
            } catch(const std::exception& e){ // may fail due invalid credentials etc
                ss << e.what();
            }
            dequeueDownload(); // pop the object from the front of the queue whether failed or not - we should manually retry
        }
        cout << "S3: downloadFromQueue complete" << endl; cout.flush();
    } else { ss << "S3: downloadQueue is empty" << endl; cout.flush(); }
    return ss.str();
}

string S3::deleteFromQueue(std::shared_ptr<Aws::S3::S3Client> s3_client){
    std::ostringstream ss;
    std::scoped_lock<std::mutex> guard(mtx);
    for(auto item: deleteQueue){
        Aws::String objectName(item);
        try {
            ss << deleteObject(s3_client, objectName, BUCKET_NAME);
        } catch(const std::exception& e){ // may fail due invalid credentials etc
            ss << e.what();
            continue; // go to the next item, but do not remove failed item from queue
        }
        dequeueDelete(); // if success, pop the object from the front of the queue
    }
    cout << ss.str(); cout.flush();
    return ss.str();
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
        remoteObjects.clear(); remoteObjectMap.clear(); // erase old remoteObjects vector/maps
        Aws::Vector<Aws::S3::Model::Object> object_list =
            list_objects_outcome.GetResult().GetContents();
        if(object_list.empty()){
            response << "S3: bucket " << BUCKET_NAME << " is empty" << std::endl;
        } else {
            response << "S3: Files on S3 bucket " << BUCKET_NAME << ":" << std::endl;
            for (auto const &s3_object : object_list)
            {   
                const char customTimeFormat[] = "%dd-%MM-%y %H:%m"; // does not work - need to figure out how to format this, then put as param to ToLocalTimeString()
                auto modtime = s3_object.GetLastModified().ToLocalTimeString(Aws::Utils::DateFormat::RFC822);
                response << s3_object.GetKey() << " : " << modtime << std::endl;
                remoteObjects.push_back(s3_object.GetKey().c_str());
                remoteObjectMap.emplace(s3_object.GetKey().c_str(), modtime);
            }
        }
    } else {
        response << "S3: listObjects error: " <<
            list_objects_outcome.GetError().GetExceptionName() << " " <<
            list_objects_outcome.GetError().GetMessage() << std::endl;
        throw std::runtime_error(list_objects_outcome.GetError().GetMessage().c_str());
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

std::unordered_map<string, string> S3::getObjectMap(){
    try {
        callAPI("listObjects"); // populate remoteObjects vector by calling listObjects via API
    } catch(const std::exception& e){ // listObjects may fail due invalid credentials etc
        throw;
    }
    return remoteObjectMap;
}

bool S3::uploadObject(std::shared_ptr<Aws::Transfer::TransferManager> transferManager, const Aws::String& bucketName, const std::string& path, const std::string& objectName){
    // encrypt file into temporary object
    string localEncryptedPath = encloned::TEMP_FILE_LOCATION + objectName;
    //cout << "localEncryptedPath: " << localEncryptedPath << endl;

    // measure timing of file encryption
    auto t1 = std::chrono::high_resolution_clock::now();
    int result = Encryption::encryptFile(localEncryptedPath.c_str(), path.c_str(), daemon->getKey());
    auto t2 = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>( t2 - t1 ).count();
    std::cout << "S3: encrypted " << path << " in " << duration << " microseconds" << endl;
    
    Aws::String awsPath(localEncryptedPath);
    Aws::String awsObjectName(objectName);

    auto uploadHandle = transferManager->UploadFile(awsPath, bucketName, awsObjectName, "", Aws::Map<Aws::String, Aws::String>());
    uploadHandle->WaitUntilFinished();
    if(uploadHandle->GetStatus() == Aws::Transfer::TransferStatus::COMPLETED){
        cout << "S3: Upload of " << path << " as " << objectName << " successful" << endl;
        assert(uploadHandle->GetBytesTotalSize() == uploadHandle->GetBytesTransferred()); // verify upload expected length of data
        remote->uploadSuccess(path, objectName, remoteID); // set remoteExists flag
        fs::remove(localEncryptedPath); // remove temporary encrypted object on local fs
        return true;
    } else {
        std::ostringstream error;
        error << "S3: Upload of " << path << " as " << objectName << " failed with status: " << uploadHandle->GetStatus() << " (" << uploadHandle->GetLastError().GetMessage() << ")" << endl;
        cout << error.str();
        throw std::runtime_error(error.str());
    }
    return false;
}

string S3::downloadObject(std::shared_ptr<Aws::Transfer::TransferManager> transferManager, const Aws::String& bucketName, const std::string& writeToPath, const std::string& objectName, std::time_t& originalModTime, std::string& targetPath){
    std::ostringstream ss;
    
    // the directory we want to download to
    if(targetPath.back() != '/'){ targetPath.append("/"); } // ensure path is terminated by a trailing slash
    string downloadDir = targetPath;

    // split path into directory path and filename
    std::size_t found = writeToPath.find_last_of("/");
    string dirPath = downloadDir + writeToPath.substr(0,found+1); // put 
    string fileName = writeToPath.substr(found+1);
    cout << "path is " << dirPath << ", fileName is " << fileName << endl;
    string downloadPath = dirPath + fileName;

    // need to create the full directory structure at path if it's doesn't already exist, or download will say successful but not save to disk
    try {
        fs::create_directories(dirPath);
        cout << "Created directories for path: " << dirPath << endl;
    }
    catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }
    
    Aws::String awsObjectName(objectName);
    // set temporary location
    string localEncryptedPath = encloned::TEMP_FILE_LOCATION + fileName;
    cout << "localEncryptedPath: " << localEncryptedPath << endl;
    Aws::String awsWriteToFile(localEncryptedPath);

    auto downloadHandle = transferManager->DownloadFile(bucketName, awsObjectName, awsWriteToFile);
    downloadHandle->WaitUntilFinished();
    if(downloadHandle->GetStatus() == Aws::Transfer::TransferStatus::COMPLETED){
        if (downloadHandle->GetBytesTotalSize() == downloadHandle->GetBytesTransferred()) {
            ss << "S3: Download of " << objectName.substr(0, 10) << "... to " << awsWriteToFile << " successful" << endl;

            // decrypt temporary file to download location and measure timing
            auto t1 = std::chrono::high_resolution_clock::now();
            int result = Encryption::decryptFile(downloadPath.c_str(), localEncryptedPath.c_str(), daemon->getKey());
            auto t2 = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>( t2 - t1 ).count();

            if(result == 0){
                // verify hash
                string downloadedFileHash = Encryption::hashFile(downloadPath);
                ss << "S3: Decryption of " << objectName.substr(0, 10) << "... to " << downloadPath;
                if(remote->getWatch()->verifyHash(objectName, downloadedFileHash)){ // hash matches stored filehash
                    ss << " successful (" << duration << " microseconds) - file hash verified" << endl;
                    fs::remove(localEncryptedPath); // remove temporary object on local fs
                } else {
                    ss << " failed - unable to verify hash" << endl;
                    fs::remove(localEncryptedPath); // remove temporary object on local fs
                    fs::remove(downloadPath); // remove decrypted object
                    cout << ss.str(); throw std::runtime_error(ss.str());
                }
            } else {
                ss << "S3: Decryption of " << objectName << " to " << downloadPath << " failed" << endl;
                cout << ss.str(); throw std::runtime_error(ss.str());
            }
            
            // set the modtime back to the original value
            fs::path fsPath = downloadPath.c_str();
            std::filesystem::file_time_type fsModtime = std::chrono::system_clock::from_time_t(originalModTime);
            fs::last_write_time(fsPath, fsModtime); 

            cout << ss.str();
            return ss.str();
        } else {
            ss << "S3: Bytes downloaded did not equal requested number of bytes: " << downloadHandle->GetBytesTotalSize() << downloadHandle->GetBytesTransferred() << std::endl;
            cout << ss.str();
            throw std::runtime_error(ss.str());
        }
    } else {
        ss << "S3: Download of " << objectName << " to " << awsWriteToFile << " failed with message: " << downloadHandle->GetStatus() << " (" << downloadHandle->GetLastError().GetMessage() << ")" << endl;
        cout << ss.str();
        throw std::runtime_error(ss.str());
    }

/*  RETRY DOWNLOAD CODE

    size_t retries = 0; // retry download if failed (up to 5 times)
    while (downloadHandle->GetStatus() == Aws::Transfer::TransferStatus::FAILED && retries++ < 5)
    {
        std::cout << "S3: Retrying download again. Attempt " << retries << " of 5" << std::endl;
        transferManager->RetryDownload(downloadHandle);
        downloadHandle->WaitUntilFinished();
    }
    ss << "S3: Download of " << objectName << " to " << awsWriteToFile << " failed after maximum retry attempts" << std::endl; 
    
*/

    cout << ss.str();
    return ss.str();
}

string S3::downloadObject(std::shared_ptr<Aws::Transfer::TransferManager> transferManager, const Aws::String& bucketName, const std::string& writeToPath, const std::string& objectName){
    std::ostringstream ss;
    
    Aws::String awsObjectName(objectName);
    // set temporary location
    string localEncryptedPath = encloned::TEMP_FILE_LOCATION + writeToPath;
    cout << "localEncryptedPath: " << localEncryptedPath << endl;
    Aws::String awsWriteToFile(localEncryptedPath);

    auto downloadHandle = transferManager->DownloadFile(bucketName, awsObjectName, awsWriteToFile);
    downloadHandle->WaitUntilFinished();
    if(downloadHandle->GetStatus() == Aws::Transfer::TransferStatus::COMPLETED){
        if (downloadHandle->GetBytesTotalSize() == downloadHandle->GetBytesTransferred()) {
            ss << "S3: Download of " << objectName.substr(0, 10) << "... to " << awsWriteToFile << " successful" << endl;

            // decrypt temporary file to download location
            if(Encryption::decryptFile(writeToPath.c_str(), localEncryptedPath.c_str(), daemon->getKey()) == 0){
                fs::remove(localEncryptedPath); // remove temporary object on local fs
            } else {
                ss << "S3: Decryption of " << objectName << " to " << writeToPath << " failed" << endl;
                cout << ss.str(); throw std::runtime_error(ss.str());
            }
            cout << ss.str();
            return ss.str();
        } else {
            ss << "S3: Bytes downloaded did not equal requested number of bytes: " << downloadHandle->GetBytesTotalSize() << downloadHandle->GetBytesTransferred() << std::endl;
            cout << ss.str();
            throw std::runtime_error(ss.str());
        }
    } else {
        ss << "S3: Download of " << objectName << " to " << awsWriteToFile << " failed with message: " << downloadHandle->GetStatus() << " (" << downloadHandle->GetLastError().GetMessage() << ")" << endl;
        cout << ss.str();
        throw std::runtime_error(ss.str());
    }
    cout << ss.str();
    return ss.str();
}

string S3::deleteObject(std::shared_ptr<Aws::S3::S3Client> s3_client, const Aws::String& objectName, const Aws::String& fromBucket){
    std::ostringstream ss;
    Aws::S3::Model::DeleteObjectRequest request;

    request.WithKey(objectName).WithBucket(fromBucket);

    Aws::S3::Model::DeleteObjectOutcome result = s3_client->DeleteObject(request);

    if (!result.IsSuccess()){   
        auto err = result.GetError();
        ss << "S3: Delete of " << objectName << " failed (" << err.GetExceptionName() << ": " << err.GetMessage() << ")" << endl;
        cout << ss.str();
        throw std::runtime_error(ss.str());
    } else {
        ss << "S3: Delete of " << objectName << " successful" << endl;
    }
    return ss.str();
}

// legacy upload/download below - not using TransferManager

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