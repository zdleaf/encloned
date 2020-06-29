#include <enclone/Socket.h>

Socket::Socket(std::atomic_bool *runThreads){ // constructor
    this->runThreads = runThreads;
}

Socket::~Socket(){ // destructor
    closeSocket();
}

void Socket::execThread(){
    openSocket();
}

void Socket::openSocket(){
    ::unlink("/tmp/enclone"); // remove previous binding
    io::local::stream_protocol::endpoint ep("/tmp/enclone");
    io::local::stream_protocol::acceptor acceptor(io_context, ep);
    io::local::stream_protocol::socket socket(io_context);
    acceptor.accept(socket);   
    std::cout << "Socket open" << std::endl; 
}

void Socket::closeSocket(){
    
}