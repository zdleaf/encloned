#include <enclone/Watch.h>

Watch::Watch(std::shared_ptr<DB> db, std::atomic_bool *runThreads, encloned* daemon){ // constructor
    this->db = db; // set DB handle
    this->runThreads = runThreads;
    this->daemon = daemon;
}

Watch::~Watch(){ // destructor
    execQueuedSQL(); // execute any pending SQL before closing
}

void Watch::setPtr(std::shared_ptr<Remote> remote){
    this->remote = remote;
}

void Watch::execThread(){
    restoreDB();
    while(*runThreads){
        for(int i = 0; i<5; i++){ // takes 5x2s before next = 10s
            for(int i = 0; i<5; i++){ // takes 5x1s before next = 5s
                //cout << "Watch: Scanning for file changes..." << endl; cout.flush();
                scanFileChange();
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            execQueuedSQL();
            std::this_thread::sleep_for(std::chrono::seconds(2));
            //indexBackup();
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
        indexBackup();
    }
}

std::unordered_map<string, std::vector<FileVersion>>* Watch::getFileIndex(){
    return &fileIndex;
}

string Watch::addWatch(string path, bool recursive){
    std::scoped_lock<std::mutex> guard(mtx);
    std::stringstream response;
    fs::file_status s = fs::status(path);
    if(!fs::exists(s)){                 // file/directory does not exist
        response << "Watch: " << path << " does not exist" << endl;
    } else if(fs::is_directory(s)){     // adding a directory to watch
        response << addDirWatch(path, recursive);
    } else if(fs::is_regular_file(s)){  // adding a regular file to watch
        response << addFileWatch(path);
    } else {                            // any other file type, e.g. IPC pipe
        response << "Watch: " << path << " does not exist" << endl;
    }
    std::cout << response.str(); cout.flush();
    return response.str();
}

string Watch::addDirWatch(string path, bool recursive){
    auto result = dirIndex.insert({path, recursive});
    std::stringstream response;
    if(result.second){ // check if insertion was successful i.e. result.second = true (false when already exists in map)
        response << "Watch: "<< "Added watch to directory: " << path << endl;
        sqlQueue << "INSERT or IGNORE INTO dirIndex (PATH, RECURSIVE) VALUES ('" << path << "'," << (recursive ? "TRUE" : "FALSE") << ");"; // queue SQL insert
        for (const auto & entry : fs::directory_iterator(path)){ // iterate through all directory entries
            fs::file_status s = fs::status(entry);
            if(fs::is_directory(s) && recursive) { // check recursive flag before adding directories recursively
                //cout << "Recursively adding: " << entry.path() << endl;
                response << addDirWatch(entry.path().string(), true); 
            } else if(fs::is_regular_file(s)){
                response << addFileWatch(entry.path().string());
            } else { 
                cout << "Watch: " << "Unknown file encountered: " << entry.path().string() << endl; 
            }
        }
    } else { // duplicate - directory was not added
        response << "Watch: " << "Watch to directory already exists: " << path << endl;
    }
    return response.str();
}

string Watch::addFileWatch(string path){
    // temporary extension exclusions - ignore .swp files
    if(fs::path(path).extension() == ".swp"){
        return "ignored .swp file";
    }

    auto result = fileIndex.insert({path, std::vector<FileVersion>()});
    std::stringstream response;

    if(result.second){ // check if insertion was successful i.e. result.second = true (false when already exists in map)
        response << "Watch: " << "Added watch to file: " << path << endl;
        addFileVersion(path);
    } else { // duplicate
        response << "Watch: " << "Watch to file already exists: " << path << endl;
    }
    return response.str();
}

void Watch::addFileVersion(std::string path){
    auto fileVector = &fileIndex[path];
    std::time_t modtime = fsLastMod(path);
    string pathHash = Encryption::hashPath(path); // compute unique filename hash for file
    string fileHash = Encryption::hashFile(path); // compute a hash of the file contents
    fileVector->push_back(FileVersion{modtime, pathHash, fileHash}); // create new FileVersion struct object and push to back of vector
    
    pathHashIndex.insert(std::make_pair(pathHash, std::make_pair(path, modtime))); // hash file path
    
    cout << "Watch: " << "Added file version: " << path << " with filename hash: " << pathHash.substr(0,10) << "..." << " modtime: " << modtime << " file hash: " << fileHash.substr(0,10) << "..." << endl;

    // queue for upload on remote and insertion into DB
    sqlQueue << "INSERT or IGNORE INTO fileIndex (PATH, MODTIME, PATHHASH, FILEHASH, LOCALEXISTS) VALUES ('" << path << "'," << modtime << ",'" << pathHash << "','" << fileHash << "',TRUE" << ");"; // if successful, queue an SQL insert into DB
    remote->queueForUpload(path, pathHash, modtime); 
}

string Watch::delWatch(string path, bool recursive){
    std::scoped_lock<std::mutex> guard(mtx);
    std::stringstream response;
    fs::file_status s = fs::status(path);
    if(!fs::exists(s)){                 // file/directory does not exist
        response << "Watch: " << path << " does not exist" << endl;
    } else if(fs::is_directory(s)){     // adding a directory to watch
        response << delDirWatch(path, recursive);
    } else if(fs::is_regular_file(s)){  // adding a regular file to watch
        response << delFileWatch(path);
    } else {                            // any other file type, e.g. IPC pipe
        response << "Watch: " << path << " does not exist" << endl;
    }
    std::cout << response.str(); cout.flush();
    return response.str();
}

string Watch::delDirWatch(string path, bool recursive){
    std::stringstream response;

    for (const auto & entry : fs::directory_iterator(path)){ // iterate through all directory entries
        fs::file_status s = fs::status(entry);
        if(fs::is_directory(s) && recursive) { // check recursive flag before removing directories recursively
            //cout << "Recursively adding: " << entry.path() << endl;
            response << delDirWatch(entry.path().string(), true); 
        } else if(fs::is_regular_file(s)){
            response << delFileWatch(entry.path().string());
        } else { 
            cout << "Watch: " << "Unknown file encountered: " << entry.path().string() << endl; 
        }
    }

    //auto elem = dirIndex.find(path);
    dirIndex.erase(path);
    sqlQueue << "DELETE FROM dirIndex WHERE PATH=\'" << path << "\';"; // queue SQL delete

    return response.str();
}

string Watch::delFileWatch(string path){
    std::stringstream response;
    auto fileVersions = fileIndex[path];
    for(auto elem: fileVersions){
        remote->queueForDelete(elem.pathHash);
        pathHashIndex.erase(elem.pathHash);
    }
    fileIndex.erase(path);
    sqlQueue << "DELETE FROM fileIndex WHERE PATH=\'" << path << "\';"; // queue SQL delete
    response << "Watch: Deleted watch from file " << path << ", this file has been queued for delete on remote backends" << endl;
    return response.str();
}

void Watch::scanFileChange(){
    std::scoped_lock<std::mutex> guard(mtx);
    // existing files that are being watched
    for(auto elem: fileIndex){
        string path = elem.first;

        // do not scan for file changes if file is already marked as not existing locally
        if(elem.second.back().localExists == false){ break; } 

        // if file has been deleted, but is still marked as existing locally
        if(!fs::exists(path) && elem.second.back().localExists == true){ 
            cout << "Watch: " << "File no longer exists: " << path << endl;
            
            fileIndex[path].back().localExists = false;
            sqlQueue << "UPDATE fileIndex SET LOCALEXISTS = FALSE WHERE PATH ='" << path << "';"; // queue SQL update  
            
            // delete from fileIndex if ALL versions do not exist remotely - if file doesn't exist locally or remotely in any form, it is lost
/*             if(ALL VERSIONS !remoteExist){
                fileIndex.erase(path);
                sqlQueue << "DELETE from fileIndex where PATH='" << path << "';"; // queue deletion from DB
            } */

            // FIX ME - when to delete from remote - if sync mode is set, otherwise no
            //remote->queueForDelete(pathHashIndex[path]);
            

            break;
        }

        // if current last_write_time of file != last saved value, file has changed
        std::time_t recentModTime = fsLastMod(path);
        //cout << "Watch: Comparing current modtime: " << recentModTime << " to saved: " << getLastModFromIdx(path) << " file: " << path << endl;
        if(recentModTime != getLastModFromIdx(path)){ 
            cout << "Watch: " << "File change detected: " << path << endl;
            fileIndex[path].back().localExists = false;
            addFileVersion(path);
        }
    }

    // check watched directories for new files and directories
    for(auto elem: dirIndex){
        if(!fs::exists(elem.first)){ // if directory has been deleted
            cout << "Watch: " << "Directory no longer exists: " << elem.first << endl;
            dirIndex.erase(elem.first); // remove watch to directory
            sqlQueue << "DELETE from dirIndex where PATH='" << elem.first << "';"; // queue deletion from DB
            break;
        }
        for (const auto &entry : fs::directory_iterator(elem.first)){ // iterate through all directory entries
            fs::file_status s = fs::status(entry);
            if(fs::is_directory(s) && elem.second) {// check recursive flag (elem.second) is true before checking if watch to dir already exists
                if(!dirIndex.count(entry.path())){   // check if directory already exists in watched map
                    cout << "Watch: " << "New directory found: " << elem.first << endl;
                    cout << addDirWatch(entry.path().string(), true);  // add new directory and any files contained within and print response
                }
            } else if(fs::is_regular_file(s)){
                if(!fileIndex.count(entry.path())){  // check if each file already exists
                    cout << "Watch: " << "New file found: " << entry.path() << endl;
                    cout << addFileWatch(entry.path().string());
                }
            }
        }
    }
}

std::time_t Watch::getLastModFromIdx(std::string path){
    return fileIndex[path].back().modtime;
}

void Watch::execQueuedSQL(){
    std::scoped_lock<std::mutex> guard(mtx);
    if(sqlQueue.rdbuf()->in_avail() != 0) { // if queue is not empty
        db->execSQL(sqlQueue.str().c_str()); // convert to c style string and execute bucket
        sqlQueue.str(""); // empty bucket
        sqlQueue.clear(); // clear error codes
    }
}

void Watch::displayWatchDirs(){
    std::scoped_lock<std::mutex> guard(mtx);
    cout << "Watched directories: " << endl;
    for(auto elem: dirIndex){
        cout << elem.first << " recursive: " << elem.second << endl;
    }
}

void Watch::displayWatchFiles(){
    std::scoped_lock<std::mutex> guard(mtx);
    cout << "Watched files: " << endl;
    for(auto elem: fileIndex){
        cout << elem.first << " last modtime: " << elem.second.back().modtime << ", # of versions: " << elem.second.size() << ", exists locally: " << elem.second.back().localExists << ", exists remotely: " << elem.second.back().remoteExists << endl;
    }
}

string Watch::listLocal(){
    return listWatchDirs() + listWatchFiles();
}

string Watch::listWatchDirs(){
    std::scoped_lock<std::mutex> guard(mtx);
    std::ostringstream ss;
    ss << "Watched directories: " << endl;
    if(dirIndex.empty()){ ss << "none" << endl; }
    else{
        for(auto elem: dirIndex){
            ss << "    " << elem.first << " recursive: " << elem.second << endl;
        }
    }
    //cout << ss.str();
    return ss.str();
}

string Watch::listWatchFiles(){
    std::scoped_lock<std::mutex> guard(mtx);
    std::ostringstream ss;
    ss << "Watched files: " << endl;
    if(fileIndex.empty()){ ss << "none" << endl; }
    else {
        for(auto elem: fileIndex){
            ss << "    " << elem.first << " last modtime: " << displayTime(elem.second.back().modtime) << ", # of versions: " << elem.second.size() << ", exists locally: " << elem.second.back().localExists << ", exists remotely: " << elem.second.back().remoteExists << endl;
        }   
    }
    //cout << ss.str();
    return ss.str();
}

string Watch::displayTime(std::time_t modtime) const{
    string time = std::asctime(std::localtime(&modtime));
    time.pop_back(); // asctime returns a '\n' on the end of the string - use str.pop_back to remove this
    return time; 
}

time_t Watch::fsLastMod(string path){
    try {
        auto fstime = fs::last_write_time(path); // get modtime from index file
        time_t modtime = decltype(fstime)::clock::to_time_t(fstime);
        return modtime;
    } catch (const std::exception &e){
        cout << "Watch: exception thrown in fsLastMod: " << e.what() << endl;
        return -1;
    }
}

std::pair<string, std::time_t> Watch::resolvePathHash(string pathHash){
    std::scoped_lock<std::mutex> guard(mtx);
    std::pair<string, std::time_t> result;
    try {
        result = pathHashIndex.at(pathHash);
    } catch (std::out_of_range &error){
        if(pathHash == indexBackupName){ return std::make_pair("index backup", indexLastMod); }
        cout << "Watch: Error: Unable to find path associated to hash " + pathHash << endl;
        throw;
    }
    return result;
}

string Watch::downloadFiles(string targetPath){ // download all files
    for(auto elem: fileIndex){
        remote->queueForDownload(elem.first, elem.second.back().pathHash, elem.second.back().modtime, targetPath);
    }
    return remote->downloadRemotes();
}

string Watch::downloadFiles(string targetPath, string pathOrHash){
    bool foundPathOrHash = false;
    // determine if 2nd parameter is a hash or a path - CLI argument does not distinguish between the two
    if(pathOrHash.length() == Encryption::getRandomFilenameLength()){ // random path hashes are 88 chars long, although we can also have a path this long - first check if file with this hash exists, else treat as a path
        for(auto elem: fileIndex){ // unordered_map
            for(auto version: elem.second){ // vector<FileVersion>
                if(version.pathHash == pathOrHash){ // found matching hash
                    foundPathOrHash = true;
                    remote->queueForDownload(elem.first, version.pathHash, version.modtime, targetPath);
                }
            }
        }
    } else {
        for(auto elem: fileIndex){
            if(pathOrHash == elem.first){ // found matching path
                foundPathOrHash = true;
                remote->queueForDownload(elem.first, elem.second.back().pathHash, elem.second.back().modtime, targetPath);
            }
        }
    }
    if(!foundPathOrHash){
        return "error: unable to find file with matching path or hash\n";
    }
    return remote->downloadRemotes();
}

bool Watch::verifyHash(string pathHash, string fileHash){
    std::scoped_lock<std::mutex> guard(mtx);
    auto result = pathHashIndex.at(pathHash);
    auto versions = fileIndex.at(std::get<0>(result));
    for(auto elem: versions){
        if(elem.pathHash == pathHash){
            if(elem.fileHash == fileHash){
                return true;
            }
        }
    }
    return false;
}

void Watch::uploadSuccess(std::string path, std::string objectName, int remoteID){
    if(objectName == indexBackupName){ return; } // if we've uploaded a backup of the index, we don't need to run this function
    try {
        auto fileVersionVector = &fileIndex.at(path);
        // set the remoteExists flag for correct entry in fileIndex
        for(auto it = fileVersionVector->rbegin(); it != fileVersionVector->rend(); ++it){ // iterate in reverse, most likely the last entry is the one we're looking for
            if(it->pathHash == objectName){ 
                it->remoteExists = true;
                // also add remoteID to list of remotes it's been uploaded to e.g. remoteLocation
            }
        }
        sqlQueue << "UPDATE fileIndex SET REMOTEEXISTS = TRUE WHERE PATHHASH ='" << objectName << "';"; // queue SQL update  
    } catch (const std::out_of_range &e){
        throw;
    } 
}

void Watch::deriveIdxBackupName(){
    std::scoped_lock<std::mutex> guard(mtx);

    // base64 encode the subkey
    auto subKey_b64 = Encryption::base64_encode(std::string(reinterpret_cast<const char*>(daemon->getSubKey())));
    // derive an Argon2id password hash string
    string hashedSubKey = Encryption::passwordKDF(subKey_b64.substr(0, subKey_b64.length()/2)); // split the key in half (32 bytes) - even if Argon2id password hash is defeated, this only results in half a subkey being exposed. even with a full derived subkey, it is not possible to determine the master key from this.

    // remove argon2id parameters from the front of the hash - "$argon2id$v=19$m=1048576,t=4,p=1$"
    size_t pos = 0;
    int count = 0;

    while(count != 3){
        pos+=1;
        pos = hashedSubKey.find("$", pos);
        count++;
    }

    if (pos == std::string::npos){
        throw std::runtime_error("error: Argon2id hashed string, unable to find delimiter\n");
    } else {
        hashedSubKey = hashedSubKey.substr(pos+1);
    }

    // b64 url encode the password hash - this is the filename used as index backup name on remote
    indexBackupName = Encryption::base64_encode(hashedSubKey);
    cout << "Used subkey to derive filename to use for index file backup: " << indexBackupName << " length: " << indexBackupName.length() << endl;

    // update db
    std::stringstream ss;
    ss << "INSERT or IGNORE INTO indexBackup (PATH, IDXNAME) VALUES ('" << db->getDbLocation() << "','" << indexBackupName << "');";
    int errorcode = db->execSQL(ss.str().c_str());
}

void Watch::indexBackup(){
    std::scoped_lock<std::mutex> guard(mtx);

    auto recentModTime = fsLastMod(db->getDbLocation());
    //cout << "DEBUG: comparing db backup old time: " << displayTime(indexLastMod) << " with new: " << displayTime(recentModTime) << endl;
    if(recentModTime > indexLastMod || indexLastMod == (time_t)-1) // index.db has changed since last backup
    {
        cout << "Watch: Backing up Index file to remote..." << endl;

        // set the modtime of the database backup in the database first
        std::stringstream ss;
        ss << "UPDATE indexBackup SET MODTIME = " << recentModTime << " WHERE IDXNAME = \"" << indexBackupName << "\";";
        int errorcode = db->execSQL(ss.str().c_str());
        if(!errorcode){ 
            // modtime of index has now changed as we've updated modtime value in database - reset the modtime
            indexLastMod = fsLastMod(db->getDbLocation()); // get modtime from database
        }

        // now backup and upload the database
        int error = db->backupDB("index.backup"); // make a temporary backup file
        if(!error){
            time_t backupLastMod = fsLastMod("index.backup");
            remote->queueForUpload("index.backup", indexBackupName, backupLastMod); // queue for encryption/upload
        } else {
            cout << "DB: sqlite index backup to temp file failed with code: " << error << endl;
        }
    }
}

string Watch::restoreIndex(string arg){
    /* 
    list remote objects and save filenames to vector
    base64 decode all of them

        - compute same subkey as above from master key (use same CONTEXT string as parameter - "INDEX___")
        - base64 encode subkey and split in half
        - decode base 64 for all filenames on remote
        - check if decoded b64 contains only a-z,A-Z,0-9,+/$ chars - this saves long verification of files that cannot be index backups
        - prefix Argon2id params to all decoded b64 strings ("$argon2id$v=19$m=1048576,t=4,p=1$")
        - use verify function to check base 64 encoded half subkey against all values until it verifies
        - some slowness is acceptable since restoring index from remote will only be done very rarely, in the event of loss of local index file.
     */
    std::stringstream response;

    auto subKey_b64 = Encryption::base64_encode(std::string(reinterpret_cast<const char*>(daemon->getSubKey()))); // base64 encode the subkey
    string password = subKey_b64.substr(0, subKey_b64.length()/2); // password used for KDF is first half of the subKey in b64

    std::unordered_map<string, string> remoteObjectMap;
    std::vector<string> verifiedIndexes;
    try {
        remoteObjectMap = remote->getObjectMap(); // get vector of objects from remote storage
        for(auto item: remoteObjectMap){ 
            auto decoded = Encryption::base64_decode(item.first);
            if(std::regex_match(decoded, std::regex("^[A-Za-z0-9\\$\\+\\/]+$"))){ // decoded strings will always be b64 with a $ sign, do not need to verify hashes that do not match this form
                if(Encryption::verifyPassword("$argon2id$v=19$m=1048576,t=4,p=1$" + decoded, password)){ // verify hash with derived password
                    cout << "Watch: verified index backup " << item.first << endl;
                    verifiedIndexes.push_back(item.first);
                    if(arg == "show"){
                        response << item.second << " : " << item.first << endl;
                    }
                } else {
                    cout << "Watch: verification failed on " << decoded << endl;
                }
            }
        }
    } catch (const std::exception& e){
        response << "Watch: Error getting remote objects: " << e.what() << endl;
    }

    if (arg.length() == 88){ // hash should be 88 chars long
        if(std::find(verifiedIndexes.cbegin(), verifiedIndexes.cend(), arg) != verifiedIndexes.cend()){ // check if provided hash matches a verified index
            response << "Restoring index backup with hash " << arg.substr(0,10) << "..." << endl;
            // download backup to index.restore
            remote->downloadNow(arg, "index.restore");
            // need to close this thread - but not able to do so from this thread
            // then need to run a --restart-daemon type command, and execute daemon checking for existence of index.restore file - rename this file if exists, and load it
            response << "Downloaded index backup from remote. Run --restart-daemon to load it" << endl;
        }
    } else if (arg != "show"){
        response << "Unknown argument provided - either 'show' or an 88 character hash of an index backup" << endl;
    }
    return response.str();
}



void Watch::restoreDB(){
    cout << "Restoring file index from DB..." << endl; cout.flush();
    restoreFileIdx();
    cout << listWatchFiles();
    cout << "Restoring directory index from DB..." << endl; cout.flush();
    restoreDirIdx();
    cout << listWatchDirs();
    cout << "Restoring index backup name from DB..." << endl; cout.flush();
    restoreIdxBackupName();

    // check we've restore an indexBackupName - if it doesn't exist then we need to derive it
    if(indexBackupName.empty()){ // if the indexBackup doesn't exist i.e. db location doesn't exist in map, then derive one
        indexLastMod = -1; // reset the last mod time of the index
        try {
            deriveIdxBackupName();
        } catch (std::exception &e) {
            cout << e.what() << endl;
        }
    }
}

void Watch::restoreFileIdx(){
    const char getFiles[] = "SELECT * FROM fileIndex;";

    int rc, i, ncols;
    sqlite3_stmt *stmt;
    const char *tail;
    rc = sqlite3_prepare(db->getDbPtr(), getFiles, strlen(getFiles), &stmt, &tail);
    if(rc != SQLITE_OK) {
        fprintf(stderr, "restoreFileIdx: SQL error: %s\n", sqlite3_errmsg(db->getDbPtr()));
    }

    rc = sqlite3_step(stmt);
    ncols = sqlite3_column_count(stmt);

    while(rc == SQLITE_ROW) {
        string path = string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        std::time_t modtime = (time_t)sqlite3_column_int(stmt, 1);
        string pathHash = string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        string fileHash = string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
        bool localExists = sqlite3_column_int(stmt, 4);
        bool remoteExists = sqlite3_column_int(stmt, 5);

        mtx.lock();
            if(fileIndex.find(path) == fileIndex.end()){ // if entry for path does not exist
                //cout << path << " does not exist in fileIndex - adding and init vector.." << endl;
                fileIndex.insert({  path, std::vector<FileVersion>{ // create entry and initialise vector 
                                        FileVersion{    modtime,  // brace initialisation of first object
                                                        pathHash, 
                                                        fileHash,
                                                        localExists,
                                                        remoteExists  }
                                        }
                                }); 
            } else {
                //cout << path << " exists, attempting to push to vector.." << endl;
                auto fileVector = &fileIndex[path]; // get a pointer to the vector associated to the path
                fileVector->push_back(FileVersion{modtime, pathHash, ""}); // push a FileVersion struct to the back of the vector
                // this should retain the correct ordering in the vector of oldest = first in vector, most recent = last in vector
            }
            
            pathHashIndex.insert(std::make_pair(pathHash, std::make_pair(path, modtime))); // also insert into reverse lookup table
        mtx.unlock();

        rc = sqlite3_step(stmt);
    }

    sqlite3_finalize(stmt);
}

void Watch::restoreDirIdx(){
    const char getDirs[] = "SELECT * FROM dirIndex;";

    int rc, i, ncols;
    sqlite3_stmt *stmt;
    const char *tail;
    rc = sqlite3_prepare(db->getDbPtr(), getDirs, strlen(getDirs), &stmt, &tail);
    if(rc != SQLITE_OK) {
        fprintf(stderr, "restoreDirIdx: SQL error: %s\n", sqlite3_errmsg(db->getDbPtr()));
    }

    rc = sqlite3_step(stmt);
    ncols = sqlite3_column_count(stmt);

    while(rc == SQLITE_ROW) {
        string path = string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        int recursiveFlag = sqlite3_column_int(stmt, 1);

        mtx.lock();
            dirIndex.insert({path, recursiveFlag});      // insert path and recursive flag into dirIndex
        mtx.unlock();

        rc = sqlite3_step(stmt);
    }

    sqlite3_finalize(stmt);
}

void Watch::restoreIdxBackupName(){
    const char getName[] = "SELECT * FROM indexBackup;";

    int rc, i, ncols;
    sqlite3_stmt *stmt;
    const char *tail;
    rc = sqlite3_prepare(db->getDbPtr(), getName, strlen(getName), &stmt, &tail);
    if(rc != SQLITE_OK) {
        fprintf(stderr, "restoreDirIdx: SQL error: %s\n", sqlite3_errmsg(db->getDbPtr()));
    }

    rc = sqlite3_step(stmt);
    ncols = sqlite3_column_count(stmt);

    while(rc == SQLITE_ROW) {
        string path = string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        string idxName = string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        std::time_t modtime = (time_t)sqlite3_column_int(stmt, 2);

        mtx.lock();
            indexBackupName = idxName;
            indexLastMod = modtime;
            //cout << "DEBUG: restored db backup modtime: " << displayTime(indexLastMod) << endl;
        mtx.unlock();

        cout << "Restored index backup name: " << idxName << endl;
        rc = sqlite3_step(stmt);
    }

    sqlite3_finalize(stmt);
}

/* LEGACY CODE
void Watch::fileAttributes(const fs::path& path){
    fs::file_status s = fs::status(path);
    // alternative: switch(s.type()) { case fs::file_type::regular: ...}
    if(fs::is_regular_file(s)) std::cout << " is a regular file\n";
    if(fs::is_directory(s)) std::cout << " is a directory\n";
    if(fs::is_block_file(s)) std::cout << " is a block device\n";
    if(fs::is_character_file(s)) std::cout << " is a character device\n";
    if(fs::is_fifo(s)) std::cout << " is a named IPC pipe\n";
    if(fs::is_socket(s)) std::cout << " is a named IPC socket\n";
    if(fs::is_symlink(s)) std::cout << " is a symlink\n";
    if(!fs::exists(s)) std::cout << " does not exist\n";
    if(fs::is_empty(path)){ std::cout << "empty\n"; } else { std::cout << "not empty\n"; };

    // size
    try {
        std::cout << "size: " << fs::file_size(path) << endl; // attempt to get size of a file
    } catch(fs::filesystem_error& e) { // e.g. is a directory, no size
        std::cout << e.what() << '\n';
    }

    // last modification time
    //std::cout << "File write time is " << displayTime(fs::last_write_time(path)) << endl;
} 

void Watch::listDir(string path){
    for (const auto & entry : fs::directory_iterator(path)){
        cout << entry.path();
        fileAttributes(entry);
    }
}
*/