#include <encloned/encloned.hpp>

int main() {
  if (!fs::exists("key")) {
    cout << "Encryption key does not exist - please run \"enclone "
            "--generate-key\" first"
         << endl;
    return 1;
  }
  encloned daemon;
  daemon.execLoop();
}

encloned::encloned() {
  runThreads = true;
  daemonMtxPtr = &daemonMtx;

  // check for existence of index.restore file and rename to index.db to load it
  if (fs::exists("index.restore")) {
    if (fs::exists("index.db")) {  // if index.db already exists, throw
                                   // exception - we do not want data loss
      throw std::runtime_error(
          "index.restore file exists, remove or backup index.db before "
          "restoring. Remove or rename index.restore to abort restoring "
          "backup.");
    } else {
      fs::rename("index.restore", "index.db");
    }
  }

  db = std::make_shared<DB>();
  socket = std::make_shared<Socket>(&runThreads);
  remote = std::make_shared<Remote>(&runThreads, this);
  watch = std::make_shared<Watch>(db, &runThreads, this);

  remote->setPtr(watch);
  watch->setPtr(remote);
  socket->setPtr(watch);
  socket->setPtr(remote);

  // set path to this executable
  char result[PATH_MAX];
  ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
  if (count != -1) {
    daemonPath = dirname(result);
  }

  // make tmp directory for encrypted files before upload
  try {
    fs::create_directories(TEMP_FILE_LOCATION);
    cout << "Created directory " << TEMP_FILE_LOCATION << endl;
  } catch (std::exception& e) {
    std::cout << e.what() << std::endl;
  }

  Encryption::initSodium();
}

encloned::~encloned() {}

int encloned::execLoop() {
  if (loadEncryptionKey()) {
    cout << "Error loading encryption key from file" << endl;
    return 1;
  }

  if (deriveSubKey()) {
    cout << "Error deriving sub-key from master encryption key" << endl;
    return 1;
  } else {
    cout << "Derived sub-key from master encryption key" << endl;
  }

  cout << "Starting Watch thread..." << endl;
  cout.flush();
  // start a thread scanning for filesystem changes
  std::thread watchThread{&Watch::execThread, watch};
  // detach threads, we not want to wait for them to finish before continuing
  // execThread() loops until runThreads == false
  watchThread.detach();

  cout << "Starting Remote thread..." << endl;
  cout.flush();
  // start a thread for the remote handler
  std::thread remoteThread{&Remote::execThread, remote};
  remoteThread.detach();

  cout << "Starting Socket thread..." << endl;
  cout.flush();
  // start a thread listening on a local socket
  std::thread socketThread{&Socket::execThread, socket};
  socketThread.detach();

  while (1) {
    // do nothing
  }
  return 0;
}

unsigned char* const encloned::getKey() { return key; }

string const encloned::getSubKey_b64() {
  return Encryption::base64_encode(subKey, sizeof subKey);
}

void encloned::addWatch(string path, bool recursive) {
  watch->addWatch(path, recursive);
}

void encloned::displayWatches() {
  watch->displayWatchDirs();
  watch->displayWatchFiles();
}

int encloned::loadEncryptionKey() {
  if (fs::file_size("key") != crypto_secretstream_xchacha20poly1305_KEYBYTES) {
    cout << "Error loading encryption key - key size is incorrect" << endl;
    return 1;
  }
  std::streampos size;
  char memblock[crypto_secretstream_xchacha20poly1305_KEYBYTES];

  std::ifstream file("key", std::ios::in | std::ios::binary | std::ios::ate);
  if (file.is_open()) {
    size = file.tellg();
    file.seekg(0, std::ios::beg);
    file.read(memblock, size);
    file.close();

    memcpy(key, memblock, sizeof memblock);
    cout << "Successfully loaded encryption key from key file" << endl;
  } else {
    cout << "Error loading encryption key - please check \"key\" and try again"
         << endl;
    return 1;
  }
  return 0;
}

int encloned::deriveSubKey() {  // derive subkey from master key
  return crypto_kdf_derive_from_key(subKey, sizeof subKey, 1, "INDEX___", key);
}
