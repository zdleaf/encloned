#include <encloned/Encryption.hpp>

void Encryption::initSodium() {
  if (sodium_init() != 0) {
    cout << "Encryption: Error initialising libsodium" << endl;
  } else {
    cout << "Encryption: libsodium initialised" << endl;
  }
}

int Encryption::encryptFile(
    const char *target_file, const char *source_file,
    const unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES]) {
  unsigned char buf_in[CHUNK_SIZE];
  unsigned char
      buf_out[CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES];
  unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
  crypto_secretstream_xchacha20poly1305_state st;
  FILE *fp_t, *fp_s;
  unsigned long long out_len;
  size_t rlen;
  int eof;
  unsigned char tag;

  fp_s = fopen(source_file, "rb");
  fp_t = fopen(target_file, "wb");
  crypto_secretstream_xchacha20poly1305_init_push(&st, header, key);
  fwrite(header, 1, sizeof header, fp_t);

  do {
    rlen = fread(buf_in, 1, sizeof buf_in, fp_s);
    eof = feof(fp_s);
    tag = eof ? crypto_secretstream_xchacha20poly1305_TAG_FINAL : 0;
    crypto_secretstream_xchacha20poly1305_push(&st, buf_out, &out_len, buf_in,
                                               rlen, NULL, 0, tag);
    fwrite(buf_out, 1, (size_t)out_len, fp_t);
  } while (!eof);

  fclose(fp_t);
  fclose(fp_s);

  return 0;
}

int Encryption::decryptFile(
    const char *target_file, const char *source_file,
    const unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES]) {
  unsigned char
      buf_in[CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES];
  unsigned char buf_out[CHUNK_SIZE];
  unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
  crypto_secretstream_xchacha20poly1305_state st;
  FILE *fp_t, *fp_s;
  unsigned long long out_len;
  size_t rlen;
  int eof;
  int ret = -1;
  unsigned char tag;

  fp_s = fopen(source_file, "rb");
  fp_t = fopen(target_file, "wb");
  fread(header, 1, sizeof header, fp_s);
  if (crypto_secretstream_xchacha20poly1305_init_pull(&st, header, key) != 0) {
    goto ret;  // incomplete header
  }
  do {
    rlen = fread(buf_in, 1, sizeof buf_in, fp_s);
    eof = feof(fp_s);
    if (crypto_secretstream_xchacha20poly1305_pull(
            &st, buf_out, &out_len, &tag, buf_in, rlen, NULL, 0) != 0) {
      goto ret;  // corrupted chunk
    }
    if (tag == crypto_secretstream_xchacha20poly1305_TAG_FINAL && !eof) {
      goto ret;  // premature end (end of file reached before the end of the
                 // stream)
    }
    fwrite(buf_out, 1, (size_t)out_len, fp_t);
  } while (!eof);

  ret = 0;
ret:
  fclose(fp_t);
  fclose(fp_s);
  return ret;
}

string Encryption::randomString(std::size_t length) {
  const std::string CHARACTERS =
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-"
      "_";  // same as base64 URL
  std::random_device random_device;
  std::mt19937 generator(random_device());
  std::uniform_int_distribution<> distribution(0, CHARACTERS.size() - 1);

  std::string randomString;
  for (std::size_t i = 0; i < length; ++i)
    randomString += CHARACTERS[distribution(generator)];

  return randomString;
}

string Encryption::hashPath(const string path) {
  return randomString(RANDOM_FILENAME_LENGTH);
}

const int Encryption::getRandomFilenameLength() {
  return RANDOM_FILENAME_LENGTH;
}

// hash a file in chunks of BUFFER_SIZE
string Encryption::hashFile(const string path) {
  unsigned char out[FILE_HASH_SIZE];
  unsigned char buf[BUFFER_SIZE];
  char hex[(FILE_HASH_SIZE * 2) + 1];
  size_t read;

  crypto_generichash_state state;
  crypto_generichash_init(&state, NULL, 0, FILE_HASH_SIZE);

  std::ifstream inputFile(path, std::ios::binary);

  if (!inputFile.is_open()) {
    std::cout << "Encryption: failed to open path for file hashing: " << path
              << endl;
  } else {
    while (inputFile) {
      inputFile.read((char *)buf, BUFFER_SIZE);
      read = inputFile.gcount();  // # of bytes read
      if (!read) {
        break;
      }
      crypto_generichash_update(&state, buf, read);
    }
  }
  crypto_generichash_final(&state, out, FILE_HASH_SIZE);
  sodium_bin2hex(hex, sizeof hex, out, FILE_HASH_SIZE);
  return hex;
}

std::string Encryption::base64_encode(const std::string &in) {
  std::string out;
  int val = 0, valb = -6;
  size_t len = in.length();
  unsigned int i = 0;
  for (i = 0; i < len; i++) {
    unsigned char c = in[i];
    val = (val << 8) + c;
    valb += 8;
    while (valb >= 0) {
      out.push_back(base64_url_alphabet[(val >> valb) & 0x3F]);
      valb -= 6;
    }
  }
  if (valb > -6) {
    out.push_back(base64_url_alphabet[((val << 8) >> (valb + 8)) & 0x3F]);
  }
  return out;
}

std::string Encryption::base64_encode(unsigned char const *bytes_to_encode,
                                      unsigned int in_len) {
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] =
          ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] =
          ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for (i = 0; (i < 4); i++) ret += base64_url_alphabet[char_array_4[i]];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 3; j++) char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] =
        ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] =
        ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++) ret += base64_url_alphabet[char_array_4[j]];

    while ((i++ < 3)) ret += '=';
  }
  return ret;
}

std::string Encryption::base64_decode(const std::string &in) {
  std::string out;
  std::vector<int> T(256, -1);
  unsigned int i;
  for (i = 0; i < 64; i++) T[base64_url_alphabet[i]] = i;

  int val = 0, valb = -8;
  for (i = 0; i < in.length(); i++) {
    unsigned char c = in[i];
    if (T[c] == -1) break;
    val = (val << 6) + T[c];
    valb += 6;
    if (valb >= 0) {
      out.push_back(char((val >> valb) & 0xFF));
      valb -= 8;
    }
  }
  return out;
}

string Encryption::deriveKey(string password) {  // with a random salt
  unsigned char salt[crypto_pwhash_SALTBYTES + 2];
  randombytes_buf(salt, sizeof salt);
  string salt_b64 = base64_encode(salt, sizeof salt);
  return deriveKey(password, salt_b64);
}

// derive a key from password, using a specified salt
// return is b64_key + b64_salt = 88 chars
string Encryption::deriveKey(string password, string salt_b64) {
  unsigned char salt[crypto_pwhash_SALTBYTES + 2];  // resultng b64 encode is 24
  unsigned char newKey[48];  // b64 encode of this will be 64 chars

  string saltStr = base64_decode(salt_b64);  // get the binary data from salt
  strcpy((char *)salt, saltStr.c_str());

  if (crypto_pwhash(newKey, sizeof newKey, password.c_str(), password.length(),
                    salt, crypto_pwhash_OPSLIMIT_MODERATE,
                    crypto_pwhash_MEMLIMIT_MODERATE,
                    crypto_pwhash_ALG_DEFAULT) != 0) {
    throw std::bad_alloc();  // out of memory
  }

  auto newKey_b64 = base64_encode(newKey, sizeof newKey);

  // cout << "newKey_b64: " << newKey_b64 << " length: " << newKey_b64.length()
  // << " key size: " << sizeof newKey << endl; cout << "salt_b64: " << salt_b64
  // << " length: " << salt_b64.length() << " salt size: " << sizeof salt <<
  // endl;
  return newKey_b64 + salt_b64;
}

bool Encryption::verifyKey(string password, string saltedKey_b64) {
  string key = saltedKey_b64.substr(0, 64);
  string salt = saltedKey_b64.substr(64);

  // check if using key with salt results in the first 64 chars of the hash
  if (deriveKey(password, salt) == saltedKey_b64) {
    return true;
  }

  return false;
}
