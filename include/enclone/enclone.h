#ifndef ENCLONE_H
#define ENCLONE_H

#include <iostream>
#include <boost/asio.hpp> // unix domain local sockets

namespace asio = boost::asio;
using boost::asio::local::stream_protocol;

#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

enum { max_length = 1024 };

class enclone{
    private:
        asio::io_service io_service;
        
    public:
        enclone();
        ~enclone();

        bool connect();

};

#else // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
# error Local sockets not available on this platform.
#endif // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

#endif