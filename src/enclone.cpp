#include <enclone/enclone.h>

int main(){
    std::cout << "Starting enclone..." << std::endl;
    boost::asio::io_service io_service;
    local::stream_protocol::endpoint ep("/tmp/enclone");
    local::stream_protocol::socket socket(io_service);
    socket.connect(ep);

}