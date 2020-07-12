//
// stream_server.cpp
// ~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// g++ -lboost_system -lboost_thread -std=c++17 -pthread stream_server.cpp -o server

#include <cstdio>
#include <iostream>
#include <memory>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

using boost::asio::local::stream_protocol;
namespace io = boost::asio;

class session: public std::enable_shared_from_this<session>{
    private:
        stream_protocol::socket socket_;    // socket used to communicate with the client
        std::array<char, 1024> data_;     // buffer used to store data received from the client
        
    public:
        session(boost::asio::io_service& io_service)
            : socket_(io_service)
        {
        }

        stream_protocol::socket& socket()
        {
            return socket_;
        }

        void start(){
            socket_.async_read_some(
                boost::asio::buffer(data_),
                boost::bind(&session::handle_read,
                shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred)
                );
        }

        void handle_read(const boost::system::error_code& error,
            size_t bytes_transferred)
        {
            if (!error)
            {
            boost::asio::async_write(socket_,
                boost::asio::buffer(data_, bytes_transferred),
                boost::bind(&session::handle_write,
                    shared_from_this(),
                    boost::asio::placeholders::error));
            }
        }

        void handle_write(const boost::system::error_code& error)
        {
            if (!error)
            {
            socket_.async_read_some(boost::asio::buffer(data_),
                boost::bind(&session::handle_read,
                    shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
            }
        }
};

class server{
    public:
        server(boost::asio::io_service& io_service, io::local::stream_protocol::endpoint ep)
            :   io_service_(io_service), 
                acceptor_(io_service, ep)
        {
            std::shared_ptr<session> new_session = std::make_shared<session>(io_service_);
            acceptor_.async_accept(new_session->socket(),
                boost::bind(&server::handle_accept, this, new_session,
                boost::asio::placeholders::error));
        }

        void handle_accept(std::shared_ptr<session> new_session, const boost::system::error_code& error)
        {
            if (!error)
            {
            new_session->start();
            }

            new_session.reset(new session(io_service_));
            acceptor_.async_accept(new_session->socket(),
                boost::bind(&server::handle_accept, this, new_session,
                boost::asio::placeholders::error));
        }

    private:
        boost::asio::io_service& io_service_;
        stream_protocol::acceptor acceptor_;
};

int main(int argc, char* argv[])
{
  try
  {
    boost::asio::io_service io_service;
    ::unlink("/tmp/encloned"); // remove previous binding
    io::local::stream_protocol::endpoint ep("/tmp/encloned");
    server s(io_service, ep);
    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}

#else // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
# error Local sockets not available on this platform.
#endif // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)