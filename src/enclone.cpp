#include <enclone/enclone.h>

int main(int argc, char* argv[]){
    try {
/*     enum style_t { allow_long = = 1, allow_short = = allow_long << 1, 
            allow_dash_for_short = = allow_short << 1, 
            allow_slash_for_short = = allow_dash_for_short << 1, 
            long_allow_adjacent = = allow_slash_for_short << 1, 
            long_allow_next = = long_allow_adjacent << 1, 
            short_allow_adjacent = = long_allow_next << 1, 
            short_allow_next = = short_allow_adjacent << 1, 
            allow_sticky = = short_allow_next << 1, 
            allow_guessing = = allow_sticky << 1, 
            long_case_insensitive = = allow_guessing << 1, 
            short_case_insensitive = = long_case_insensitive << 1, 
            case_insensitive = = (long_case_insensitive | short_case_insensitive), 
            allow_long_disguise = = short_case_insensitive << 1, 
            unix_style = = (allow_short | short_allow_adjacent | short_allow_next
                    | allow_long | long_allow_adjacent | long_allow_next
                    | allow_sticky | allow_guessing 
                    | allow_dash_for_short), 
            default_style = = unix_style }; */

        // CLI arguments
        po::options_description desc("Allowed options");
        desc.add_options()
        ("help,h", "produce help message")
        ("add,a", po::value<string>(), "add a watch to a given file or directory")
        ("recursive,r", "specify watched directory should be watched recursively");
    
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

        if (vm.count("help") || (argc == 1)) {
            cout << desc << endl;
            return 0;
        }

        if (vm.count("add") && vm.count("recursive")) {
            cout << "Adding watch to path: " << vm["add"].as<string>() << endl;
            enclone e("add", vm["add"].as<string>(), true);
        } else if (vm.count("add")) {
            cout << "Adding watch to path: " << vm["add"].as<string>() << endl;
            enclone e("add", vm["add"].as<string>(), false);
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
    if (!fs::exists(path)){
        std::cerr << "path: " << path << " does not exist\n";
        return;
    }
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
        asio::io_service io_service;
        stream_protocol::socket localSocket(io_service);
        asio::local::stream_protocol::endpoint ep("/tmp/encloned");
        localSocket.connect(ep);

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