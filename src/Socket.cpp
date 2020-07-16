#include <enclone/Socket.h>

Socket::Socket(std::atomic_bool *runThreads){ // constructor
    this->runThreads = runThreads;
}

Socket::~Socket(){ // destructor
    closeSocket();
}

void Socket::setPtr(std::shared_ptr<Watch> watch){
    this->watch = watch;
}

void Socket::setPtr(std::shared_ptr<Remote> remote){
    this->remote = remote;
}

void Socket::execThread(){
    openSocket();
}

void Socket::openSocket(){
    try
    {
        ::unlink(SOCKET_FILE); // remove previous binding
        asio::local::stream_protocol::endpoint ep(SOCKET_FILE);
        Server s(io_service, ep, watch, remote);
        fs::permissions(SOCKET_FILE, fs::perms::owner_all); // restrict file permissions to the socket to owner only (+0700 rwx------)
        io_service.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }    
}

void Socket::closeSocket(){
    
}

Session::Session(asio::io_service& io_service, std::shared_ptr<Watch> watch, std::shared_ptr<Remote> remote): socket_(io_service)
{
    std::cout << "Socket: New session started..." << std::endl;
    this->watch = watch; 
    this->remote = remote;
}

Session::~Session(){
    std::cout << "Socket: Session closed..." << std::endl; 
}

stream_protocol::socket& Session::socket()
{
    return socket_;
}

void Session::start(){
    socket_.async_read_some(
        asio::buffer(data_),
        boost::bind(&Session::handle_read,
        shared_from_this(),
        asio::placeholders::error,
        asio::placeholders::bytes_transferred)
        );
}

void Session::handle_read(const boost::system::error_code& error, size_t bytes_transferred){
    if (!error)
    {
        string response;
        string delimiter = "|";

        std::string request(std::begin(data_), data_.begin()+bytes_transferred);
        auto delimiterPosition = request.find(delimiter);

        // DEBUGGING
        std::string cmd = request.substr(0, delimiterPosition); cout << "Command is: \"" << cmd << "\"" << endl; // get command that appears before delimter
        std::string path = request.substr(delimiterPosition+1); cout << "Args are: \"" << path << "\"" << endl;

        // input handling here - need a parser
        if(cmd == "add"){
            response = watch->addWatch(path, false) + ";";
        } else if (cmd == "addr"){
            response = watch->addWatch(path, true) + ";";
        } else if (cmd == "listLocal"){
            response = watch->listLocal() + ";";
        } else if (cmd == "listRemote"){
            response = remote->listObjects() + ";";
        }

        cout << "Socket: Sending response to socket: \"" << response.substr(0, 20) << "...\"" << endl;

        asio::async_write(
            socket_,
            asio::buffer(response.c_str(), response.length()),
            boost::bind(&Session::handle_write,
            shared_from_this(),
            asio::placeholders::error)
        );
    }
}

void Session::handle_write(const boost::system::error_code& error){
    if (!error)
    {       
            socket_.async_read_some(asio::buffer(data_),
            boost::bind(&Session::handle_read,
            shared_from_this(),
            asio::placeholders::error,
            asio::placeholders::bytes_transferred));
    }
}

Server::Server(asio::io_service& io_service, asio::local::stream_protocol::endpoint ep, std::shared_ptr<Watch> watch, std::shared_ptr<Remote> remote)
    :   io_service_(io_service), 
        acceptor_(io_service, ep)
{   
    std::cout << "Socket: Local socket open..." << std::endl;
    this->watch = watch;
    this->remote = remote;
    std::shared_ptr<Session> newSession = std::make_shared<Session>(io_service_, watch, remote);
    acceptor_.async_accept(newSession->socket(),
        boost::bind(&Server::handle_accept, this, newSession,
        asio::placeholders::error));
}

void Server::handle_accept(std::shared_ptr<Session> newSession, const boost::system::error_code& error){
    if (!error)
    { 
        newSession->start(); 
    }
    
    newSession.reset(new Session(io_service_, watch, remote));
    acceptor_.async_accept(newSession->socket(),
        boost::bind(&Server::handle_accept, this, newSession,
        asio::placeholders::error));
}