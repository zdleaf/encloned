## enclone

## Installation from source
CMake is required to build the project.
On Debian/Ubuntu: `sudo apt install cmake`

In order to connect with AWS S3, the AWS CLI package is required. This can be installed with:
```
sudo apt install awscli
```
Credentials for an AWS account must be saved in the `~/.aws/credentials` file

The AWS C++ SDK is also required. Full installation instructions can be found at https://docs.aws.amazon.com/sdk-for-cpp/v1/developer-guide/setup.html or a shortened version for Debian/Ubuntu below:

Install dependencies required for AWS SDK:
```
sudo apt-get install libcurl4-openssl-dev libssl-dev uuid-dev zlib1g-dev libpulse-dev
```

Git clone the source for the SDK:
```
git clone https://github.com/aws/aws-sdk-cpp.git
```

Enter the aws-sdk-cpp directory and use CMake to configure build files for only the required libraries (s3, transfer):
```
cd aws-sdk-cpp
mkdir sdk_build
cd sdk_build
sudo cmake .. -D CMAKE_BUILD_TYPE=Release -D BUILD_ONLY="s3;transfer"
```

Then build and install the s3/transfer libraries to the system:
```
sudo make install
```

Install libsodium, SQLite3 and Boost libraries:
```
sudo apt install libsodium-dev libsqlite3-dev libboost-all-dev
```

Clone the enclone source code:
```
git clone https://github.com/zdleaf/enclone.git
```

Enter the directory, create a build directory, configure build files and compile
```
cd enclone
mkdir build; cd build
cmake ..
make
```
The binaries ```enclone``` and ```encloned``` are then available. 

To start, enter a directory where you want to store the index.db and master encryption keys.
e.g.
```
mkdir ~/.enclone
cd ~/.enclone
```
and run the following:
```
/path/to/enclone --generate-key
```

followed by `/path/to/encloned` to start the daemon.


