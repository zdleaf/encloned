#ifndef SOCKET_H
#define SOCKET_H

#include <iostream>
#include <boost/asio.hpp> // unix domain local sockets

#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

namespace io = boost::asio;

class Socket {
    private:
        io::io_context io_context;
        
    public:
        Socket();
        ~Socket();

        void openSocket();
        void closeSocket();
};

#else // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
# error Local sockets not available on this platform.
#endif // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

#endif