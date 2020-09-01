## enclone
enclone is a solution for securely storing and syncing static data between Linux, and cloud storage platforms such as AWS S3. The focus is on ensuring the confidentiality, authenticity and integrity of data at all times.

The project was developed as part of an MSc Computer Science dissertation, the full PDF paper and implementation details can be found in this directory.

The software consists of a daemon `encloned` and a control client used to interact with the daemon `enclone`.
  
## Generating encryption keys, and starting encloned
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
followed by `/path/to/encloned` in the same directory to start the daemon.
## Using enclone
Commands are passed to the daemon via the `enclone` utility.

To track an individual file or a directory (non-recursively) use `--add-watch (-a)`:
```
enclone --add-watch /path/to/file
enclone -a /path/to/dir
```
or multiple single files/directories:
```
enclone -a /path/to/file1 -a /path/to/file2 -a /path/to/a/dir
```

To track a directory recursively, use `--add-recursive (-A)`:
```
enclone --add-recursive /path/to/dir
enclone -A /path/to/dir2 -A /path/to/dir2
```

Deletion of tracked files/directories can be completed using `--del-watch (-d)` and `--del-recursive (-D)`

To list all files and directories that are tracked locally, use `--list local` or `-l local`.

To display all files available on remote storage, use `--list remote` or `-l remote`.

To restore all files, use `--restore (-r)` with a `--target (-t)` flag:
```
enclone --restore all --target /path/to/dl/to
```

To restore an individual file, provide the filename provided by `--list remote`:
```
enclone --restore ckg430CAEcb3xbRwnNX_aOp-X8b5mYjUyZpmKS3rqrA0RxX0Q-BiDt-bNyt30 --target /path/to/dl/to
```

To list the backed up indexes on remote storage that can be restored, use `--restore-index (-i)`:
```
enclone --restore-index show
```

To restore an index, provide the filename to `--restore-index (-i)`:
```
enclone --restore-index xVy-AVUWsP-HNcKmuuvrtQj5lGG4e-X3T4xz5s_TK_dBKSxRjxBGPvF1dci5uO9VHg9WkiXSCa_e0WhY1L35NshN
``` 

Full command list:
```
  -h [ --help ]              display this help message

  -l [ --list ] arg          show currently tracked/available files

                                local: show all tracked local files
                                remote: show all available remote files

  -a [ --add-watch ] arg     add a watch to a given path (file or directory)
  -A [ --add-recursive ] arg recursively add a watch to a directory
  -d [ --del-watch ] arg     delete a watch from a given path (file or
                             directory)
  -D [ --del-recursive ] arg recursively delete all watches in a directory

  -r [ --restore ] arg       restore files from remote

                                all: restore latest versions of all files
                                /path/to/file: restore the latest version of a
                                               file at specified path
                                filehash: restore a specific version of a file
                                          by providing the full hash as output
                                          by --list remote

  -t [ --target ] arg        specify a target path to restore files to

  -i [ --restore-index ] arg restore an index/database from remote storage
                                show: show all remotely backed up indexes
                                filehash: restore a specific index by giving
                                          the encrypted filename

  -k [ --generate-key ]      generate an encryption key
  -c [ --clean-up ]          remove items from remote S3 which do not have a
                             corresponding entry in fileIndex
```

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


