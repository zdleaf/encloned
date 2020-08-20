#include <enclone/Encryption.h>

void Encryption::initSodium(){
    if (sodium_init() != 0) {
        cout << "Encryption: Error initialising libsodium" << endl;
    } else { 
        cout << "Encryption: libsodium initialised" << endl; 
    }
}

int Encryption::encryptFile(const char *target_file, 
    const char *source_file, 
    const unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES])
{
    unsigned char  buf_in[CHUNK_SIZE];
    unsigned char  buf_out[CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES];
    unsigned char  header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
    crypto_secretstream_xchacha20poly1305_state st;
    FILE          *fp_t, *fp_s;
    unsigned long long out_len;
    size_t         rlen;
    int            eof;
    unsigned char  tag;

    fp_s = fopen(source_file, "rb");
    fp_t = fopen(target_file, "wb");
    crypto_secretstream_xchacha20poly1305_init_push(&st, header, key);
    fwrite(header, 1, sizeof header, fp_t);
    do {
        rlen = fread(buf_in, 1, sizeof buf_in, fp_s);
        eof = feof(fp_s);
        tag = eof ? crypto_secretstream_xchacha20poly1305_TAG_FINAL : 0;
        crypto_secretstream_xchacha20poly1305_push(&st, buf_out, &out_len, buf_in, rlen,
                                                   NULL, 0, tag);
        fwrite(buf_out, 1, (size_t) out_len, fp_t);
    } while (! eof);
    fclose(fp_t);
    fclose(fp_s);
    return 0;
}

int Encryption::decryptFile(const char *target_file, 
    const char *source_file,
    const unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES])
{
    unsigned char  buf_in[CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES];
    unsigned char  buf_out[CHUNK_SIZE];
    unsigned char  header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
    crypto_secretstream_xchacha20poly1305_state st;
    FILE          *fp_t, *fp_s;
    unsigned long long out_len;
    size_t         rlen;
    int            eof;
    int            ret = -1;
    unsigned char  tag;

    fp_s = fopen(source_file, "rb");
    fp_t = fopen(target_file, "wb");
    fread(header, 1, sizeof header, fp_s);
    if (crypto_secretstream_xchacha20poly1305_init_pull(&st, header, key) != 0) {
        goto ret; /* incomplete header */
    }
    do {
        rlen = fread(buf_in, 1, sizeof buf_in, fp_s);
        eof = feof(fp_s);
        if (crypto_secretstream_xchacha20poly1305_pull(&st, buf_out, &out_len, &tag,
                                                       buf_in, rlen, NULL, 0) != 0) {
            goto ret; /* corrupted chunk */
        }
        if (tag == crypto_secretstream_xchacha20poly1305_TAG_FINAL && ! eof) {
            goto ret; /* premature end (end of file reached before the end of the stream) */
        }
        fwrite(buf_out, 1, (size_t) out_len, fp_t);
    } while (! eof);

    ret = 0;
ret:
    fclose(fp_t);
    fclose(fp_s);
    return ret;
}

string Encryption::randomString(std::size_t length)
{
    const std::string CHARACTERS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<> distribution(0, CHARACTERS.size()-1);

    std::string randomString;
    for (std::size_t i = 0; i < length; ++i) randomString += CHARACTERS[distribution(generator)];

    return randomString;
}

string Encryption::hashPath(const string path){
    return randomString(RANDOM_FILENAME_LENGTH);
}

const int Encryption::getRandomFilenameLength(){
    return RANDOM_FILENAME_LENGTH;
}

string Encryption::hashFile(const string path){ // hash a file in chunks of BUFFER_SIZE
    unsigned char out[FILE_HASH_SIZE];
    unsigned char buf[BUFFER_SIZE];
    char hex[(FILE_HASH_SIZE * 2) + 1];
    size_t read;

    crypto_generichash_state state;
    crypto_generichash_init(&state, NULL, 0, FILE_HASH_SIZE);

    std::ifstream inputFile(path, std::ios::binary);

    if (!inputFile.is_open()) {
        std::cout << "Encryption: failed to open path for file hashing: " << path << endl;
    } else {
        while(inputFile){
            inputFile.read((char *)buf, BUFFER_SIZE);
            read = inputFile.gcount(); // # of bytes read
            if (!read){ break; }
            crypto_generichash_update(&state,buf,read);
        }
    }
    crypto_generichash_final(&state, out, FILE_HASH_SIZE);
    sodium_bin2hex(hex, sizeof hex, out, FILE_HASH_SIZE);
    return hex;
}

std::string Encryption::base64_encode(const std::string & in){
  std::string out;
  int val =0, valb=-6;
  size_t len = in.length();
  unsigned int i = 0;
  for (i = 0; i < len; i++) {
    unsigned char c = in[i];
    val = (val<<8) + c;
    valb += 8;
    while (valb >= 0) {
      out.push_back(base64_url_alphabet[(val>>valb)&0x3F]);
      valb -= 6;
    }
  }
  if (valb > -6) {
    out.push_back(base64_url_alphabet[((val<<8)>>(valb+8))&0x3F]);
  }
  return out;
}

std::string Encryption::base64_decode(const std::string & in){
  std::string out;
  std::vector<int> T(256, -1);
  unsigned int i;
  for (i =0; i < 64; i++) T[base64_url_alphabet[i]] = i;

  int val = 0, valb = -8;
  for (i = 0; i < in.length(); i++) {
    unsigned char c = in[i];
    if (T[c] == -1) break;
    val = (val<<6) + T[c];
    valb += 6;
    if (valb >= 0) {
      out.push_back(char((val>>valb)&0xFF));
      valb -= 8;
    }
  }
  return out;
}

string Encryption::passwordKDF(string password){
    char hashed_password[crypto_pwhash_STRBYTES];

    if (crypto_pwhash_str
    (hashed_password, password.c_str(), password.length(), crypto_pwhash_OPSLIMIT_SENSITIVE, crypto_pwhash_MEMLIMIT_SENSITIVE) != 0){
        throw std::bad_alloc(); // out of memory
    }

    auto hashedPasswordStr = std::string(reinterpret_cast<const char*>(hashed_password));

    return hashedPasswordStr;
}

bool Encryption::verifyPassword(string hash, string password){
    if (crypto_pwhash_str_verify(hash.c_str(), password.c_str(), password.length()) != 0) {
        return false;
    } else {
        return true;
    }
}

/* legacy hash code

string Encryption::sha256(const string str)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);
    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
} 

*/