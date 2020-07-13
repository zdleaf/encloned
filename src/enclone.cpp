#include <enclone/enclone.h>

int main(int argc, char* argv[]){
    cout << argv[0] << ", " << argv[1] << ", " << argv[2] << endl;
    if (argc < 2){
        std::cerr << "Usage: enclone command\n";
        return 1;
    }
    std::cout << "Starting enclone..." << std::endl;
    if (std::string(argv[1]) == "add"){
        cout << "add true" << endl;
        if (argc < 3){ std::cerr << "Usage: enclone add <path>\n"; }
        else { 
            enclone e("add", argv[2]); 
        }
    }
}

enclone::enclone(string cmd, string path){
    if (!fs::exists(path)){
        std::cerr << "path: " << path << " does not exist\n";
        return;
    }
    if (cmd == "add"){
        addWatch(path, true);
    }
    //connect();
}

enclone::~enclone(){
    
}

bool enclone::addWatch(string path, bool recursive){
    try
    {
        asio::io_service io_service;
        stream_protocol::socket localSocket(io_service);
        asio::local::stream_protocol::endpoint ep("/tmp/encloned");
        localSocket.connect(ep);

        std::cout << "Enter message: ";
        char request[path.length()];
        strcpy(request, path.c_str());
        size_t request_length = std::strlen(request);
        asio::write(localSocket, asio::buffer(request, request_length));

        char reply[max_length];
        size_t reply_length = asio::read(localSocket, asio::buffer(reply, request_length));
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