#include <enclone/enclone.h>

int main(int argc, char* argv[]){
    std::cout << "Starting enclone..." << std::endl;
    enclone e;
    /*     if (argc != 2)
        {
        std::cerr << "Usage: stream_client <file>\n";
        return 1;
        } */

}

enclone::enclone(){
    connect();
}

enclone::~enclone(){
    
}

bool enclone::connect(){
    try
    {
        stream_protocol::socket localSocket(io_service);
        asio::local::stream_protocol::endpoint ep("/tmp/encloned");
        localSocket.connect(ep);

        std::cout << "Enter message: ";
        char request[max_length];
        std::cin.getline(request, max_length);
        size_t request_length = std::strlen(request);
        asio::write(localSocket, asio::buffer(request, request_length));

        char reply[max_length];
        size_t reply_length = asio::read(localSocket,
            asio::buffer(reply, request_length));
        std::cout << "Reply is: ";
        std::cout.write(reply, reply_length);
        std::cout << "\n";
        return true;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
        return false;
    }
}