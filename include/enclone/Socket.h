#ifndef SOCKET_H
#define SOCKET_H

#include <iostream>
#include <filesystem>
#include <boost/asio.hpp> // unix domain local sockets
#include <boost/bind.hpp>

#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

namespace fs = std::filesystem;
namespace asio = boost::asio;
using boost::asio::local::stream_protocol;

class Session;
class Server;

class Socket {
    private:
        const char* SOCKET_FILE = "/tmp/encloned";
        //io::io_context io_context;
        asio::io_service io_service;

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

class Session: public std::enable_shared_from_this<Session>{
    private:
        stream_protocol::socket socket_;    // socket used to communicate with the client
        std::array<char, 1024> data_;     // buffer used to store data received from the client
        
    public:
        Session(asio::io_service& io_service);
        ~Session();

        stream_protocol::socket& socket();

        void start();

        void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
        void handle_write(const boost::system::error_code& error);
};

class Server{
    private:
        asio::io_service& io_service_;
        stream_protocol::acceptor acceptor_;

    public:
        Server(asio::io_service& io_service, asio::local::stream_protocol::endpoint ep);

        void handle_accept(std::shared_ptr<Session> newSession, const boost::system::error_code& error);
};

#else // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
# error Local sockets not available on this platform.
#endif // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

#endif