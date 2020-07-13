#ifndef ENCLONE_H
#define ENCLONE_H

#include <iostream>
#include <string>
#include <filesystem>
#include <boost/asio.hpp> // unix domain local sockets

namespace fs = std::filesystem;
namespace asio = boost::asio;
using boost::asio::local::stream_protocol;
using std::string;
using std::cout;
using std::endl;

#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

enum { max_length = 1024 };

class enclone{
    private:
        asio::io_service io_service;
        bool addWatch(string path, bool recursive);
        
    public:
        enclone(string cmd, string path);
        ~enclone();

        bool connect();

};

#else // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
# error Local sockets not available on this platform.
#endif // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

#endif