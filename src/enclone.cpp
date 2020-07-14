#include <enclone/enclone.h>

void conflicting_options(const po::variables_map& vm, const char* opt1, const char* opt2){
    if (vm.count(opt1) && !vm[opt1].defaulted() 
        && vm.count(opt2) && !vm[opt2].defaulted())
        throw std::logic_error(string("Conflicting options '") 
                          + opt1 + "' and '" + opt2);
}

int main(int argc, char* argv[]){
    try {
        // CLI arguments
        po::options_description desc("Allowed options");
        desc.add_options()
        ("help,h", "produce help message")
        ("add-watch,a", po::value<string>(), "add a watch to a given file or directory")
        ("recursive,r", "specify watched directory should be watched recursively")
        ("del-watch,d", po::value<string>(), "add a watch to a given file or directory");
    
        // store/parse arguments
        po::variables_map vm;        
        po::store(
            po::parse_command_line(argc, argv, desc, 
                po::command_line_style::unix_style |
                po::command_line_style::allow_dash_for_short |
                po::command_line_style::case_insensitive |
                po::command_line_style::long_allow_adjacent |
                po::command_line_style::long_allow_next |
                po::command_line_style::short_allow_adjacent |
                po::command_line_style::short_allow_next |
                po::command_line_style::allow_long_disguise
                ), vm);

        po::notify(vm);

        conflicting_options(vm, "remove", "add");

        if (vm.count("help") || (argc == 1)) {
            cout << desc << endl;
            return 0;
        }

        if (vm.count("add-watch") && vm.count("recursive")) {
            cout << "Adding watch to path: " << vm["add-watch"].as<string>() << endl;
            enclone e("add", vm["add-watch"].as<string>(), true);
        } else if (vm.count("add-watch")) {
            cout << "Adding watch to path: " << vm["add-watch"].as<string>() << endl;
            enclone e("add", vm["add-watch"].as<string>(), false);
        }

    } 
    catch(std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    catch(...) {
        std::cerr << "Exception of unknown type!\n";
    }
    return 0;
}

enclone::enclone(string cmd, string path, bool recursive){ // recursive=false by default
/*     if (!fs::exists(path)){
        std::cerr << "path: " << path << " does not exist\n";
        return;
    } */
    if (cmd == "add" && recursive){
        addWatch(path, true);
    } else if (cmd == "add" && !recursive){
        addWatch(path, false);
    }
}

enclone::~enclone(){
    
}

bool enclone::addWatch(string path, bool recursive){
    try
    {
        // init socket connection
        asio::io_service io_service;
        stream_protocol::socket localSocket(io_service);
        asio::local::stream_protocol::endpoint ep("/tmp/encloned");
        localSocket.connect(ep);

        // send request
        char request[path.length()];
        strcpy(request, path.c_str());
        size_t request_length = std::strlen(request);
        asio::write(localSocket, asio::buffer(request, request_length));

        // read response
        boost::asio::streambuf response; // read_until requires a dynamic buffer to read into
        asio::read_until(localSocket, response, ";"); // all responses are suffixed with ";" - read until this delimiter
        
        std::string reply{buffers_begin(response.data()), buffers_end(response.data())-1}; // get string response from buffer, remove delimiter
        cout << reply;

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