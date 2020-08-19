#ifndef ENCLONE_H
#define ENCLONE_H

#include <iostream>
#include <string>
#include <stdexcept>
#include <iterator>
#include <vector>

#include <filesystem>
#include <fstream>

#include <boost/asio.hpp> // unix domain local sockets
#include <boost/program_options.hpp> // CLI arguments parsing

#include <sodium.h>

namespace fs = std::filesystem;
namespace asio = boost::asio;
namespace po = boost::program_options;
using boost::asio::local::stream_protocol;
using std::string;
using std::cout;
using std::endl;

#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

enum { max_length = 2048 };

class enclone{
    public:
        enclone(const int& argc, char** const argv);
        ~enclone();

    private:
        asio::io_service io_service;
        
        int showOptions(const int& argc, char** const argv);
        void conflictingOptions(const po::variables_map& vm, const char* opt1, const char* opt2);

        bool sendRequest(string request);

        bool addWatch(string path, bool recursive);
        bool delWatch(string path, bool recursive);
        bool listLocal();
        bool listRemote();
        bool restoreFiles(string targetPath);
        bool restoreFiles(string targetPath, string pathOrHash);
        bool restoreIndex();
        bool cleanRemote();

        void generateKey(); // generate encryption key to file

        std::vector<string> toAdd{}; // paths to watch
        std::vector<string> toRecAdd{};   // recursive directories to watch
        std::vector<string> toDel{}; // paths to delete watches to

        std::vector<string> toRestore{}; // paths to restore
        


};

#else // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
# error Local sockets not available on this platform.
#endif // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

#endif