#include <enclone/enclone.h>

int main(){
    std::cout << "Starting enclone..." << std::endl;
/*     boost::asio::io_service io_service;
    local::stream_protocol::endpoint ep("/tmp/enclone");
    local::stream_protocol::socket socket(io_service);
    socket.connect(ep); */

        try
    {
    /*     if (argc != 2)
        {
        std::cerr << "Usage: stream_client <file>\n";
        return 1;
        } */

        asio::io_service io_service;

        stream_protocol::socket localSocket(io_service);
        asio::local::stream_protocol::endpoint ep("/tmp/encloned");
        localSocket.connect(ep);

        using namespace std; // For strlen.
        std::cout << "Enter message: ";
        char request[max_length];
        std::cin.getline(request, max_length);
        size_t request_length = strlen(request);
        asio::write(localSocket, asio::buffer(request, request_length));

        char reply[max_length];
        size_t reply_length = asio::read(localSocket,
            asio::buffer(reply, request_length));
        std::cout << "Reply is: ";
        std::cout.write(reply, reply_length);
        std::cout << "\n";
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

}