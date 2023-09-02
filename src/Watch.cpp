#include <encloned/Watch.h>

Watch::Watch(std::shared_ptr<DB> db, std::atomic_bool *runThreads,
             encloned *daemon) {
  this->db = db;
  this->runThreads = runThreads;
  this->daemon = daemon;
}

Watch::~Watch() {
  execQueuedSQL();  // execute any pending SQL before closing
}

void Watch::setPtr(std::shared_ptr<Remote> remote) { this->remote = remote; }

void Watch::execThread() {
  restoreDB();
  while (*runThreads) {
    for (int i = 0; i < 5; i++) {    // takes 5x2s before next = 10s
      for (int i = 0; i < 5; i++) {  // takes 5x1s before next = 5s
        // cout << "Watch: Scanning for file changes..." << endl; cout.flush();
        scanFileChange();
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
      execQueuedSQL();
      std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    indexBackup();
  }
}

std::unordered_map<string, std::vector<FileVersion>> *Watch::getFileIndex() {
  return &fileIndex;
}

string Watch::addWatch(string path, bool recursive) {
  std::scoped_lock<std::mutex> guard(mtx);
  std::stringstream response;
  fs::file_status s = fs::status(path);
  if (!fs::exists(s)) {  // file/directory does not exist
    response << "Watch: " << path << " does not exist" << endl;
  } else if (fs::is_directory(s)) {  // adding a directory to watch
    response << addDirWatch(path, recursive);
  } else if (fs::is_regular_file(s)) {  // adding a regular file to watch
    response << addFileWatch(path);
  } else {  // any other file type, e.g. IPC pipe
    response << "Watch: " << path << " does not exist" << endl;
  }
  std::cout << response.str();
  cout.flush();
  return response.str();
}

string Watch::addDirWatch(string path, bool recursive) {
  auto result = dirIndex.insert({path, recursive});
  std::stringstream response;
  // check if insertion was successful i.e. result.second = true
  // (false when already exists in map)
  if (result.second) {
    response << "Watch: "
             << "Added watch to directory: " << path << endl;
    sqlQueue << "INSERT or IGNORE INTO dirIndex (PATH, RECURSIVE) VALUES ('"
             << path << "'," << (recursive ? "TRUE" : "FALSE")
             << ");";
    for (const auto &entry : fs::directory_iterator(path)) {
      fs::file_status s = fs::status(entry);
      if (fs::is_directory(s) && recursive) {
        // cout << "Recursively adding: " << entry.path() << endl;
        response << addDirWatch(entry.path().string(), true);
      } else if (fs::is_regular_file(s)) {
        response << addFileWatch(entry.path().string());
      } else {
        cout << "Watch: "
             << "Unknown file encountered: " << entry.path().string() << endl;
      }
    }
  } else {  // duplicate - directory was not added
    response << "Watch: "
             << "Watch to directory already exists: " << path << endl;
  }
  return response.str();
}

string Watch::addFileWatch(string path) {
  // temporary extension exclusions - ignore .swp files
  if (fs::path(path).extension() == ".swp") {
    return "ignored .swp file";
  }

  auto result = fileIndex.insert({path, std::vector<FileVersion>()});
  std::stringstream response;

  // check if insertion was successful i.e. result.second = true
  // (false when already exists in map)
  if (result.second) {
    response << "Watch: "
             << "Added watch to file: " << path << endl;
    addFileVersion(path);
  } else {  // duplicate
    response << "Watch: "
             << "Watch to file already exists: " << path << endl;
  }
  return response.str();
}

void Watch::addFileVersion(std::string path) {
  auto fileVector = &fileIndex[path];
  std::time_t modtime = fsLastMod(path);
  // compute unique filename hash for file
  string pathHash = Encryption::hashPath(path);
  // compute a hash of the file contents
  string fileHash = Encryption::hashFile(path);
  // create new FileVersion struct object and push to back of vector
  fileVector->push_back(FileVersion{modtime, pathHash, fileHash});

  pathHashIndex.insert(std::make_pair(
      pathHash, std::make_pair(path, modtime)));  // hash file path

  cout << "Watch: "
       << "Added file version: " << path
       << " with filename hash: " << pathHash.substr(0, 10) << "..."
       << " modtime: " << modtime << " file hash: " << fileHash.substr(0, 10)
       << "..." << endl;

  // queue for upload on remote and insertion into DB
  sqlQueue << "INSERT or IGNORE INTO fileIndex (PATH, MODTIME, PATHHASH, "
              "FILEHASH, LOCALEXISTS) VALUES ('"
           << path << "'," << modtime << ",'" << pathHash << "','" << fileHash
           << "',TRUE"
           << ");";  // if successful, queue an SQL insert into DB
  remote->queueForUpload(path, pathHash, modtime);
}

string Watch::delWatch(string path, bool recursive) {
  std::scoped_lock<std::mutex> guard(mtx);
  std::stringstream response;
  fs::file_status s = fs::status(path);
  if (!fs::exists(s)) {  // file/directory does not exist
    response << "Watch: " << path << " does not exist" << endl;
  } else if (fs::is_directory(s)) {  // adding a directory to watch
    response << delDirWatch(path, recursive);
  } else if (fs::is_regular_file(s)) {  // adding a regular file to watch
    response << delFileWatch(path);
  } else {  // any other file type, e.g. IPC pipe
    response << "Watch: " << path << " does not exist" << endl;
  }
  std::cout << response.str();
  cout.flush();
  return response.str();
}

string Watch::delDirWatch(string path, bool recursive) {
  std::stringstream response;

  for (const auto &entry : fs::directory_iterator(path)) {
    fs::file_status s = fs::status(entry);
    if (fs::is_directory(s) && recursive) {
      // cout << "Recursively adding: " << entry.path() << endl;
      response << delDirWatch(entry.path().string(), true);
    } else if (fs::is_regular_file(s)) {
      response << delFileWatch(entry.path().string());
    } else {
      cout << "Watch: "
           << "Unknown file encountered: " << entry.path().string() << endl;
    }
  }

  dirIndex.erase(path);
  sqlQueue << "DELETE FROM dirIndex WHERE PATH=\'" << path << "\';";

  return response.str();
}

string Watch::delFileWatch(string path) {
  std::stringstream response;
  auto fileVersions = fileIndex[path];
  for (auto elem : fileVersions) {
    remote->queueForDelete(elem.pathHash);  // queue for remote deletion
    pathHashIndex.erase(elem.pathHash);
  }
  fileIndex.erase(path);
  sqlQueue << "DELETE FROM fileIndex WHERE PATH=\'" << path
           << "\';";
  response << "Watch: Deleted watch from file " << path
           << ", this file has been queued for delete on remote backends"
           << endl;
  return response.str();
}

void Watch::scanFileChange() {
  std::scoped_lock<std::mutex> guard(mtx);
  // existing files that are being watched
  for (auto elem : fileIndex) {
    string path = elem.first;

    // do not scan for file changes if file is already marked as not existing
    // locally
    if (elem.second.back().localExists == false) {
      break;
    }

    // if file has been deleted, but is still marked as existing locally
    if (!fs::exists(path) && elem.second.back().localExists == true) {
      cout << "Watch: "
           << "File no longer exists: " << path << endl;

      fileIndex[path].back().localExists = false;
      sqlQueue << "UPDATE fileIndex SET LOCALEXISTS = FALSE WHERE PATH ='"
               << path << "';";

      break;
    }

    // if current last_write_time of file != last saved value, file has changed
    std::time_t recentModTime = fsLastMod(path);
    // cout << "Watch: Comparing current modtime: " << recentModTime << " to
    // saved: " << getLastModFromIdx(path) << " file: " << path << endl;
    if (recentModTime != getLastModFromIdx(path)) {
      cout << "Watch: "
           << "File change detected: " << path << endl;
      fileIndex[path].back().localExists = false;
      addFileVersion(path);
    }
  }

  // check watched directories for new files and directories
  for (auto elem : dirIndex) {
    if (!fs::exists(elem.first)) {  // if directory has been deleted
      cout << "Watch: "
           << "Directory no longer exists: " << elem.first << endl;
      dirIndex.erase(elem.first);  // remove watch to directory
      sqlQueue << "DELETE from dirIndex where PATH='" << elem.first
               << "';";
      break;
    }
    // iterate through all directory entries
    for (const auto &entry : fs::directory_iterator(elem.first)) {
      fs::file_status s = fs::status(entry);
      if (fs::is_directory(s) && elem.second) {
        // check if directory already exists in watched map
        if (!dirIndex.count(entry.path())) {
          cout << "Watch: "
               << "New directory found: " << elem.first << endl;
          // add new directory and any files contained within and print response
          cout << addDirWatch(entry.path().string(), true);
        }
      } else if (fs::is_regular_file(s)) {
        // check if each file already exists
        if (!fileIndex.count(entry.path())) {
          cout << "Watch: "
               << "New file found: " << entry.path() << endl;
          cout << addFileWatch(entry.path().string());
        }
      }
    }
  }
}

std::time_t Watch::getLastModFromIdx(std::string path) {
  return fileIndex[path].back().modtime;
}

void Watch::execQueuedSQL() {
  std::scoped_lock<std::mutex> guard(mtx);
  if (sqlQueue.rdbuf()->in_avail() != 0) {  // if queue is not empty
    db->execSQL(sqlQueue.str().c_str());
    sqlQueue.str("");  // empty bucket
    sqlQueue.clear();  // clear error codes
  }
}

void Watch::displayWatchDirs() {
  std::scoped_lock<std::mutex> guard(mtx);
  cout << "Watched directories: " << endl;
  for (auto elem : dirIndex) {
    cout << elem.first << " recursive: " << elem.second << endl;
  }
}

void Watch::displayWatchFiles() {
  std::scoped_lock<std::mutex> guard(mtx);
  cout << "Watched files: " << endl;
  for (auto elem : fileIndex) {
    cout << elem.first << " last modtime: " << elem.second.back().modtime
         << ", # of versions: " << elem.second.size()
         << ", exists locally: " << elem.second.back().localExists
         << ", exists remotely: " << elem.second.back().remoteExists << endl;
  }
}

string Watch::listLocal() { return listWatchDirs() + listWatchFiles(); }

string Watch::listWatchDirs() {
  std::scoped_lock<std::mutex> guard(mtx);
  std::ostringstream ss;
  ss << "Watched directories: " << endl;
  if (dirIndex.empty()) {
    ss << "none" << endl;
  } else {
    for (auto elem : dirIndex) {
      ss << "    " << elem.first << " recursive: " << elem.second << endl;
    }
  }
  // cout << ss.str();
  return ss.str();
}

string Watch::listWatchFiles() {
  std::scoped_lock<std::mutex> guard(mtx);
  std::ostringstream ss;
  ss << "Watched files: " << endl;
  if (fileIndex.empty()) {
    ss << "none" << endl;
  } else {
    for (auto elem : fileIndex) {
      ss << "    " << elem.first
         << " last modtime: " << displayTime(elem.second.back().modtime)
         << ", # of versions: " << elem.second.size()
         << ", exists locally: " << elem.second.back().localExists
         << ", exists remotely: " << elem.second.back().remoteExists << endl;
    }
  }
  // cout << ss.str();
  return ss.str();
}

string Watch::displayTime(std::time_t modtime) const {
  string time = std::asctime(std::localtime(&modtime));
  time.pop_back();  // asctime returns a '\n' on the end of the string - use
                    // str.pop_back to remove this
  return time;
}

time_t Watch::fsLastMod(string path) {
  try {
    auto fstime = fs::last_write_time(path);  // get modtime from index file
    auto systime = std::chrono::file_clock::to_sys(fstime);
    time_t modtime = std::chrono::system_clock::to_time_t(systime);
    return modtime;
  } catch (const std::exception &e) {
    cout << "Watch: exception thrown in fsLastMod: " << e.what() << endl;
    return -1;
  }
}

std::pair<string, std::time_t> Watch::resolvePathHash(string pathHash) {
  std::scoped_lock<std::mutex> guard(mtx);
  std::pair<string, std::time_t> result;
  try {
    result = pathHashIndex.at(pathHash);
  } catch (std::out_of_range &error) {
    if (pathHash == indexBackupName) {
      return std::make_pair("index backup", indexLastMod);
    }
    cout << "Watch: Error: Unable to find path associated to hash " + pathHash
         << endl;
    throw;
  }
  return result;
}

string Watch::downloadFiles(string targetPath) {  // download all files
  for (auto elem : fileIndex) {
    remote->queueForDownload(elem.first, elem.second.back().pathHash,
                             elem.second.back().modtime, targetPath);
  }
  return remote->downloadRemotes();
}

string Watch::downloadFiles(string targetPath, string pathOrHash) {
  bool foundPathOrHash = false;
  // determine if 2nd parameter is a hash or a path - CLI argument does not
  // distinguish between the two
  // random path hashes are 88 chars long, so if we have a path or hash this
  // size, check if file with this hash exists, otherwise it's a path
  if (pathOrHash.length() == Encryption::getRandomFilenameLength()) {
    for (auto elem : fileIndex) {              // unordered_map
      for (auto version : elem.second) {       // vector<FileVersion>
        if (version.pathHash == pathOrHash) {  // found matching hash
          foundPathOrHash = true;
          remote->queueForDownload(elem.first, version.pathHash,
                                   version.modtime, targetPath);
        }
      }
    }
  } else {
    for (auto elem : fileIndex) {
      if (pathOrHash == elem.first) {  // found matching path
        foundPathOrHash = true;
        remote->queueForDownload(elem.first, elem.second.back().pathHash,
                                 elem.second.back().modtime, targetPath);
      }
    }
  }
  if (!foundPathOrHash) {
    return "error: unable to find file with matching path or hash\n";
  }
  return remote->downloadRemotes();
}

bool Watch::verifyHash(string pathHash, string fileHash) const {
  auto result = pathHashIndex.at(pathHash);
  auto versions = fileIndex.at(std::get<0>(result));
  for (auto elem : versions) {
    if (elem.pathHash == pathHash) {
      if (elem.fileHash == fileHash) {
        return true;
      }
    }
  }
  return false;
}

void Watch::uploadSuccess(std::string path, std::string objectName,
                          int remoteID) {
  // if we've uploaded a backup of the index, we don't need to run this function
  if (objectName == indexBackupName) {
    return;
  }
  try {
    auto fileVersionVector = &fileIndex.at(path);
    // set the remoteExists flag for correct entry in fileIndex
    // iterate in reverse, most likely the last entry is the one we're looking for
    for (auto it = fileVersionVector->rbegin(); it != fileVersionVector->rend();
         ++it) {
      if (it->pathHash == objectName) {
        it->remoteExists = true;
        // also add remoteID to list of remotes it's been uploaded to e.g.
        // remoteLocation
      }
    }
    sqlQueue << "UPDATE fileIndex SET REMOTEEXISTS = TRUE WHERE PATHHASH ='"
             << objectName << "';";
  } catch (const std::out_of_range &e) {
    throw;
  }
}

void Watch::deriveIdxBackupName() {
  std::scoped_lock<std::mutex> guard(mtx);

  // derive a b64 encoding key + random salt using the subkey as a password
  indexBackupName = Encryption::deriveKey(daemon->getSubKey_b64());
  cout << "Used subkey to derive filename to use for index file backup: "
       << indexBackupName << " length: " << indexBackupName.length() << endl;

  // update db
  std::stringstream ss;
  ss << "INSERT or IGNORE INTO indexBackup (PATH, IDXNAME) VALUES ('"
     << db->getDbLocation() << "','" << indexBackupName << "');";
  int errorcode = db->execSQL(ss.str().c_str());
}

void Watch::indexBackup() {
  std::scoped_lock<std::mutex> guard(mtx);

  // do not backup an empty database to avoid providing a possible
  // known-plaintext pair available on the cloud
  if (dirIndex.empty() && fileIndex.empty()) {
    return;
  }

  auto recentModTime = fsLastMod(db->getDbLocation());
  // cout << "DEBUG: comparing db backup old time: " <<
  // displayTime(indexLastMod) << " with new: " << displayTime(recentModTime) <<
  // endl;
  if (recentModTime > indexLastMod ||
      indexLastMod == (time_t)-1)  // index.db has changed since last backup
  {
    cout << "Watch: Backing up Index file to remote..." << endl;

    // set the modtime of the database backup in the database first
    std::stringstream ss;
    ss << "UPDATE indexBackup SET MODTIME = " << recentModTime
       << " WHERE IDXNAME = \"" << indexBackupName << "\";";
    int errorcode = db->execSQL(ss.str().c_str());
    if (!errorcode) {
      // modtime of index has now changed as we've updated modtime value in
      // database - reset the modtime
      indexLastMod =
          fsLastMod(db->getDbLocation());  // get modtime from database
    }

    // now backup and upload the database
    int error = db->backupDB("index.backup");  // make a temporary backup file
    if (!error) {
      time_t backupLastMod = fsLastMod("index.backup");
      remote->queueForUpload("index.backup", indexBackupName, backupLastMod);
    } else {
      cout << "DB: sqlite index backup to temp file failed with code: " << error
           << endl;
    }
  }
}

string Watch::restoreIndex(string arg) {
  /*
      - list remote objects and save filenames to vector
      - compute same subkey as above from master key (use same CONTEXT string as
        parameter - "INDEX___")
      - base64 encode subkey
      - use Encryption::verifyKey(subkey, filename) on all files to check if
        file is a valid index backup
      - some slowness is acceptable since restoring index from remote will only
        be done very rarely, in the event of loss of local index file.
   */
  std::stringstream response;

  std::unordered_map<string, string> remoteObjectMap;
  string subKey_b64 = daemon->getSubKey_b64();

  // get map of objects from remote storage <objectName, modtime string>
  try {
    remoteObjectMap = remote->getObjectMap();
  } catch (const std::exception &e) {
    response << "Watch: Error getting remote objects: " << e.what() << endl;
  }

  // try and verify all files on remote, to determine if they are index backups
  if (arg == "show") {
    for (auto item : remoteObjectMap) {
      // verify if filename was computed from the subkey
      if (Encryption::verifyKey(subKey_b64, item.first)) {
        cout << "Watch: verified index backup: " << item.first << endl;
        response << item.second << " : " << item.first << endl;
      }
    }
  }

  else if (arg.length() == 88) {  // hash should be 88 chars long
    if (remoteObjectMap.find(arg) !=
        remoteObjectMap.end()) {  // check if provided hash is in object map
      // verify if filename was computed from the subkey
      if (Encryption::verifyKey(subKey_b64, arg)) {
        response << "Restoring index backup with hash " << arg.substr(0, 10)
                 << "..." << endl;
        // download and decrypt index backup to index.restore
        remote->downloadNow(arg, "index.restore");
        response
            << "Downloaded index backup from remote. Remove and backup the "
               "current index.db if required, and restart daemon to load it"
            << endl;
      }
    } else {
      response << "Unable to find hash " << arg << " on remote storage";
    }
  }

  else if (arg != "show") {
    response << "Unknown argument provided - either 'show' or an 88 character "
                "hash of an index backup"
             << endl;
  }

  return response.str();
}

void Watch::restoreDB() {
  cout << "Restoring file index from DB..." << endl;
  cout.flush();
  restoreFileIdx();
  cout << listWatchFiles();
  cout << "Restoring directory index from DB..." << endl;
  cout.flush();
  restoreDirIdx();
  cout << listWatchDirs();
  cout << "Restoring index backup name from DB..." << endl;
  cout.flush();
  restoreIdxBackupName();

  // check we've restored an indexBackupName - if it doesn't exist then we need
  // to derive it
  if (indexBackupName.empty()) {
    indexLastMod = -1;  // reset the last mod time of the index
    try {
      deriveIdxBackupName();
    } catch (std::exception &e) {
      cout << e.what() << endl;
    }
  }
}

void Watch::restoreFileIdx() {
  const char getFiles[] = "SELECT * FROM fileIndex;";

  int rc, i, ncols;
  sqlite3_stmt *stmt;
  const char *tail;
  rc =
      sqlite3_prepare(db->getDbPtr(), getFiles, strlen(getFiles), &stmt, &tail);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "restoreFileIdx: SQL error: %s\n",
            sqlite3_errmsg(db->getDbPtr()));
  }

  rc = sqlite3_step(stmt);
  ncols = sqlite3_column_count(stmt);

  while (rc == SQLITE_ROW) {
    string path =
        string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
    std::time_t modtime = (time_t)sqlite3_column_int(stmt, 1);
    string pathHash =
        string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)));
    string fileHash =
        string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3)));
    bool localExists = sqlite3_column_int(stmt, 4);
    bool remoteExists = sqlite3_column_int(stmt, 5);

    mtx.lock();
    if (fileIndex.find(path) ==
        fileIndex.end()) {  // if entry for path does not exist
      // cout << path << " does not exist in fileIndex - adding and init
      // vector.." << endl;
      fileIndex.insert(
          {path,
           std::vector<FileVersion>{
               // create entry and initialise vector
               FileVersion{modtime, pathHash, fileHash, localExists,
                           remoteExists}}});
    } else {
      // cout << path << " exists, attempting to push to vector.." << endl;
      // get a pointer to the vector associated to the path
      auto fileVector = &fileIndex[path];
      // push a FileVersion struct to the back of the vector
      fileVector->push_back(FileVersion{modtime, pathHash, ""});
      // this should retain the correct ordering in the vector of oldest = first
      // in vector, most recent = last in vector
    }

    // also insert into reverse lookup table
    pathHashIndex.insert(std::make_pair(pathHash,
                         std::make_pair(path, modtime)));
    mtx.unlock();

    rc = sqlite3_step(stmt);
  }

  sqlite3_finalize(stmt);
}

void Watch::restoreDirIdx() {
  const char getDirs[] = "SELECT * FROM dirIndex;";

  int rc, i, ncols;
  sqlite3_stmt *stmt;
  const char *tail;
  rc = sqlite3_prepare(db->getDbPtr(), getDirs, strlen(getDirs), &stmt, &tail);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "restoreDirIdx: SQL error: %s\n",
            sqlite3_errmsg(db->getDbPtr()));
  }

  rc = sqlite3_step(stmt);
  ncols = sqlite3_column_count(stmt);

  while (rc == SQLITE_ROW) {
    string path =
        string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
    int recursiveFlag = sqlite3_column_int(stmt, 1);

    mtx.lock();
    dirIndex.insert({path, recursiveFlag});
    mtx.unlock();

    rc = sqlite3_step(stmt);
  }

  sqlite3_finalize(stmt);
}

void Watch::restoreIdxBackupName() {
  const char getName[] = "SELECT * FROM indexBackup;";

  int rc, i, ncols;
  sqlite3_stmt *stmt;
  const char *tail;
  rc = sqlite3_prepare(db->getDbPtr(), getName, strlen(getName), &stmt, &tail);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "restoreDirIdx: SQL error: %s\n",
            sqlite3_errmsg(db->getDbPtr()));
  }

  rc = sqlite3_step(stmt);
  ncols = sqlite3_column_count(stmt);

  while (rc == SQLITE_ROW) {
    string path =
        string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
    string idxName =
        string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)));
    std::time_t modtime = (time_t)sqlite3_column_int(stmt, 2);

    mtx.lock();
    indexBackupName = idxName;
    indexLastMod = modtime;
    // cout << "DEBUG: restored db backup modtime: " <<
    // displayTime(indexLastMod) << endl;
    mtx.unlock();

    cout << "Restored index backup name: " << idxName << endl;
    cout.flush();
    rc = sqlite3_step(stmt);
  }

  sqlite3_finalize(stmt);
}
