#include <enclone/Socket.h>

Socket::Socket(){ // constructor
    ::unlink("/tmp/enclone"); // remove previous binding
    openSocket();
}

Socket::~Socket(){ // destructor
    closeSocket();
}

void Socket::openSocket(){
    io::local::stream_protocol::endpoint ep("/tmp/enclone");
    io::local::stream_protocol::acceptor acceptor(io_context, ep);
    io::local::stream_protocol::socket socket(io_context);
    acceptor.accept(socket);    
}

void Socket::closeSocket(){
    
}