//
// stream_client.cpp
// ~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// g++ -lboost_system -lboost_thread -std=c++17 -pthread stream_client.cpp -o client

// g++ -lboost_system -lboost_thread -std=c++17 -pthread stream_server.cpp -o server; g++ -lboost_system -lboost_thread -std=c++17 -pthread stream_client.cpp -o client

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <boost/asio.hpp>

#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

namespace asio = boost::asio;
using boost::asio::local::stream_protocol;

enum { max_length = 1024 };

int main(int argc, char* argv[])
{
  try
  {
/*     if (argc != 2)
    {
      std::cerr << "Usage: stream_client <file>\n";
      return 1;
    } */

    boost::asio::io_service io_service;
    stream_protocol::socket localSocket(io_service);
    asio::local::stream_protocol::endpoint ep("/tmp/encloned");
    localSocket.connect(ep);

    using namespace std; // For strlen.
    std::cout << "Enter message: ";
    char request[max_length];
    std::cin.getline(request, max_length);
    size_t request_length = strlen(request);
    boost::asio::write(localSocket, boost::asio::buffer(request, request_length));

    char reply[max_length];
    size_t reply_length = boost::asio::read(localSocket,
        boost::asio::buffer(reply, request_length));
    std::cout << "Reply is: ";
    std::cout.write(reply, reply_length);
    std::cout << "\n";
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