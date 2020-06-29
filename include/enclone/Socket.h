#ifndef SOCKET_H
#define SOCKET_H

#include <iostream>
#include <boost/asio.hpp> // unix domain local sockets

#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

namespace io = boost::asio;

class Socket {
    private:
        io::io_context io_context;

        // concurrency/multi-threading
        std::mutex mtx;
        std::atomic_bool *runThreads; // ptr to flag indicating if execThread should loop or close down
        
    public:
        Socket(std::atomic_bool *runThreads);
        ~Socket();

        // delete copy constructors - this class should not be copied
        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;

        void execThread();

        void openSocket();
        void closeSocket();
};

#else // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
# error Local sockets not available on this platform.
#endif // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

#endif