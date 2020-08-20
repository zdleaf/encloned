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
        ("help,h", "display this help message\n")
        ("list,l", po::value<string>(), "show currently tracked/available files\n\n"
            "   local: \tshow all tracked local files\n"
            "   remote: \tshow all available remote files\n")
        ("add-watch,a", po::value<std::vector<string>>(&toAdd)->composing(), "add a watch to a given path (file or directory)")
        ("add-recursive,A", po::value<std::vector<string>>(&toRecAdd)->composing(), "recursively add a watch to a directory")
        //("recursive,r", "specify watched directory should be watched or deleted recursively")
        ("del-watch,d", po::value<std::vector<string>>(&toDel)->composing(), "delete a watch from a given path (file or directory)")
        ("del-recursive,D", po::value<std::vector<string>>(&toDel)->composing(), "recursively delete all watches in a directory\n")
        ("restore,r", po::value<std::vector<string>>(&toRestore)->composing(), "restore files from remote\n\n"
            "   all: \trestore latest versions of all files\n"
            "   /path/to/file: \trestore the latest version of a file at specified path \n"
            "   filehash: \trestore a specific version of a file by providing the full hash as output by --list remote\n")
        ("target,t", po::value<string>(), "specify a target path to restore files to\n")
        ("restore-index,i", po::value<string>(), "restore an index/database from remote storage\n"
            "   latest: \trestore most recent index backup found on remote\n"
            "   show: \tshow all remotely backed up indexes\n"
            "   filehash: \trestore a specific index by giving the encrypted filename\n")
        ("generate-key,k", "generate an encryption key")
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
                po::command_line_style::allow_sticky
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

        if (vm.count("add-recursive")){
            for(string path: toRecAdd){
                cout << "Adding recursive watch to path: " << path << endl;
                addWatch(path, true);
            }
        }

        if (vm.count("del-watch")) {
            for(string path: toDel){
                cout << "Deleting watch to path: " << path << endl;
                delWatch(path, false);
            }
        }

        if (vm.count("del-recursive")) {
            for(string path: toDel){
                cout << "Recursively deleting watch to path: " << path << endl;
                delWatch(path, true);
            }
        }

        if (vm.count("restore")){
            string targetPath;
            if (vm.count("target")){
                targetPath = vm["target"].as<string>();
            } else {
                std::cerr << "error: please specify a --target (-t) directory to save to" << endl;
                return 1;
            }
            for(string arg: toRestore){
                if(arg == "all"){
                    restoreFiles(targetPath);
                } else {
                    restoreFiles(targetPath, arg);
                }
            }
        }

        if (vm.count("restore-index")){
            string arg = vm["restore-index"].as<string>();
            if(arg == "latest" || arg == "show" || arg.length() == 88){
                restoreIndex(arg);
            }
            else {
                std::cerr << "error: please enter 'latest', 'show' or an 88 character encrypted index name" << endl;
                return 1;
            }
            
        }

        if (vm.count("generate-key")){
            generateKey();
        }

        if (vm.count("clean-up")){
            cleanRemote();
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

bool enclone::delWatch(string path, bool recursive){
    string request;
    (recursive) ? request = "delr|" : request = "del|"; 
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

bool enclone::cleanRemote(){
    string request = "cleanRemote|";
    return sendRequest(request);
}

bool enclone::restoreFiles(string targetPath){
    string request = "restoreAll|" + targetPath;
    return sendRequest(request);
}

bool enclone::restoreFiles(string targetPath, string pathOrHash){
    string request = "restore|" + targetPath + "|" + pathOrHash;
    return sendRequest(request);
}

bool enclone::restoreIndex(string arg){
    string request = "restoreIndex|" + arg;
    return sendRequest(request);
}

void enclone::generateKey(){
    if (sodium_init() != 0) {
        cout << "Error initialising libsodium" << endl;
    }

    if(fs::exists("key")){
        cout << "Error: key already exists - please backup and remove old key before generating a new one" << endl;
    } else {
        std::ofstream output("key"); // create a blank file

        // set correct permissions before writing key data to file
        fs::permissions("key", fs::perms::owner_read | fs::perms::owner_write);

        // generate key
        unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
        crypto_secretstream_xchacha20poly1305_keygen(key);

        // write key to file
        std::ofstream file;
        file.open("key", std::ios::out | std::ios::trunc | std::ios::binary); 
        if (file.is_open())
        {
            file.write((char*)key, crypto_secretstream_xchacha20poly1305_KEYBYTES);
        }
        file.close();
        cout << "Generated encryption key and saved to this directory" << endl;
    }
}