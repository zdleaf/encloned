#include <enclone/enclone.h>

int main(int argc, char* argv[]){
    enclone enc(argc, argv);
}

enclone::enclone(const int& argc, char** const argv){
    showOptions(argc, argv);
}

enclone::~enclone(){
    
}

int enclone::showOptions(const int& argc, char** const argv){ 
    try {
        // CLI arguments
        po::options_description desc("Allowed options");
        desc.add_options()
        ("help,h", "display this help message")
        ("list,l", po::value<string>(), "show currently tracked/available files\n"
            "   local: \tshow all tracked local files\n"
            "   remote: \tshow all available remote files\n"
            "   versions: \tshow all available versions of all files on remote\n")
        ("add-watch,a", po::value<std::vector<string>>(&toAdd)->composing(), "add a watch to a given path (file or directory)")
        ("recursive-add,A", po::value<std::vector<string>>(&toRecAdd)->composing(), "recursively add a watch to a directory")
        //("recursive,r", "specify watched directory should be watched or deleted recursively")
        ("del-watch,d", po::value<std::vector<string>>(&toDel)->composing(), "delete a watch from a given path (file or directory)")
        ("recursive-del,D", po::value<std::vector<string>>(&toDel)->composing(), "recursively delete all watches in a directory")
        ("restore,r", po::value<std::vector<string>>(&toAdd)->composing(), "restore either \"all\" files on remote, or a specific file version")
        ("clean-up,c", "remove items from remote S3 which do not have a corresponding entry in fileIndex");
    
        // store/parse arguments
        po::variables_map vm;        
        po::store(
            po::parse_command_line(argc, argv, desc, 
                po::command_line_style::unix_style |
                po::command_line_style::allow_dash_for_short |
                po::command_line_style::long_allow_adjacent |
                po::command_line_style::long_allow_next |
                po::command_line_style::short_allow_adjacent |
                po::command_line_style::short_allow_next |
                po::command_line_style::allow_long_disguise
                ), vm);

        po::notify(vm);

        conflictingOptions(vm, "add-watch", "help"); // <--- change this, example of how to disallow certain options in conjuction

        if (vm.count("help") || (argc == 1)){
            cout << desc << endl;
            return 0;
        }

        if (vm.count("list")){
            string arg = vm["list"].as<string>();
            if(arg == "local"){
                listLocal();
            } else if (arg == "remote"){
                listRemote();
            } else {
                cout << "Incorrect argument to --list (-l) - enter either local or remote";
            }
        }

        if (vm.count("add-watch")) {
            for(string path: toAdd){
                cout << "Adding watch to path: " << path << endl;
                addWatch(path, false);
            }
        }

        if (vm.count("recursive-add")){
            for(string path: toRecAdd){
                cout << "Adding recursive watch to path: " << path << endl;
                addWatch(path, true);
            }
        }

        if (vm.count("del-watch")){
            cout << "Deleting watch to path: " << vm["del-watch"].as<string>() << endl;
            //enclone e("add", vm["add-watch"].as<string>(), true);
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

void enclone::conflictingOptions(const po::variables_map& vm, const char* opt1, const char* opt2){
    if (vm.count(opt1) && !vm[opt1].defaulted() 
        && vm.count(opt2) && !vm[opt2].defaulted())
        throw std::logic_error(string("Conflicting options '") 
                          + opt1 + "' and '" + opt2);
}

bool enclone::sendRequest(string request){
    try
    {
        // init socket connection
        asio::io_service io_service;
        stream_protocol::socket localSocket(io_service);
        asio::local::stream_protocol::endpoint ep("/tmp/encloned");
        localSocket.connect(ep);

        // send request
        char req[request.length()];
        strcpy(req, request.c_str());
        asio::write(localSocket, asio::buffer(req, request.length()));

        // read response
        boost::asio::streambuf response; // read_until requires a dynamic buffer to read into
        asio::read_until(localSocket, response, ";"); // all responses are suffixed with ";" - read until this delimiter
        
        std::string reply{buffers_begin(response.data()), buffers_end(response.data())-1}; // get string response from buffer, remove delimiter
        cout << reply;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
        return false;
    }
    return true;
}

bool enclone::addWatch(string path, bool recursive){
    string request;
    (recursive) ? request = "addr|" : request = "add|"; 
    request += path;

    return sendRequest(request);
}

bool enclone::listLocal(){
    string request = "listLocal|";

    return sendRequest(request);
}

bool enclone::listRemote(){
    string request = "listRemote|";

    return sendRequest(request);
}
