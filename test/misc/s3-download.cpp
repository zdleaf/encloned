// New way
Aws::Transfer::TransferManagerConfiguration transferConfig;
transferConfig.s3Client = SessionClient;

std::shared_ptr<Aws::Transfer::TransferHandle> requestPtr(nullptr);

transferConfig.downloadProgressCallback =
        [](const Aws::Transfer::TransferManager*, const Aws::Transfer::TransferHandle& handle)
{
    std::cout << "\r" << "<AWS DOWNLOAD> Download Progress: " << static_cast<int>(handle.GetBytesTransferred() * 100.0 / handle.GetBytesTotalSize()) << " Percent " << handle.GetBytesTransferred() << " bytes\n";
};

Aws::Transfer::TransferManager transferManager(transferConfig);

requestPtr = transferManager.DownloadFile(bucket.c_str(), keyName.c_str(), [&destination](){

    Aws::FStream *stream = Aws::New<Aws::FStream>("s3file", destination, std::ios_base::out);
    stream->rdbuf()->pubsetbuf(NULL, 0);

    return stream; });

requestPtr->WaitUntilFinished();

size_t retries = 0;
//just make sure we don't fail because a download part failed. (e.g. network problems or interuptions)
while (requestPtr->GetStatus() == Aws::Transfer::TransferStatus::FAILED && retries++ < 5)
{
    std::cout << "<AWS DOWNLOAD> FW Download trying download again!" << std::endl;
    transferManager.RetryDownload(requestPtr);
    requestPtr->WaitUntilFinished();
}

// Check status
if ( requestPtr->GetStatus() == Aws::Transfer::TransferStatus::COMPLETED ) {
    if ( requestPtr->GetBytesTotalSize() == requestPtr->GetBytesTransferred() ) {
        std::cout << "<AWS DOWNLOAD> Get FW success!" << std::endl;
        exit(0);
    }
    else {
        std::cout << "<AWS DOWNLOAD> Get FW failed - Bytes downloaded did not equal requested number of bytes: " << requestPtr->GetBytesTotalSize() << requestPtr->GetBytesTransferred() << std::endl;
        exit(1);
    }
}
else {
    std::cout << "<AWS DOWNLOAD> Get FW failed - download was never completed even after retries" << std::endl;
    exit(1);
}