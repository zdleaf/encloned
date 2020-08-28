#include <iostream>
#include <sodium.h>
#include <fstream>
#include <stdio.h>
#include <cstring>
#include <string>
#include <filesystem>

// g++ path-encryption.cpp -o path -lsodium -lstdc++fs -std=c++17 

using std::string;
using std::cout;
using std::endl;
namespace fs = std::filesystem;

#define BASE64_VARIATION sodium_base64_VARIANT_URLSAFE
unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES];

void initSodium(){
    if (sodium_init() != 0) {
        cout << "Encryption: Error initialising libsodium" << endl;
    } else { 
        cout << "Encryption: libsodium initialised" << endl; 
    }
}

int loadEncryptionKey(){
    if(fs::file_size("/home/zach/enclone/key") != crypto_secretstream_xchacha20poly1305_KEYBYTES){
        cout << "Error loading encryption key - key size is incorrect" << endl;
        return 1;         
    }
    std::streampos size;
    char memblock[crypto_secretstream_xchacha20poly1305_KEYBYTES];

    std::ifstream file ("/home/zach/enclone/key", std::ios::in|std::ios::binary|std::ios::ate);
    if (file.is_open())
    {
        size = file.tellg();
        file.seekg(0, std::ios::beg);
        file.read(memblock, size);
        file.close();

        memcpy(key, memblock, sizeof memblock); // copy into the key member variable
        cout << "Successfully loaded encryption key from key file" << endl;
    }
    else { 
        cout << "Error loading encryption key - please check \"key\" and try again" << endl;
        return 1; 
    }
    return 0;
}

const char base64_url_alphabet[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '_'
};

std::string base64_decode(const std::string & in) {
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

std::string base64_encode(const std::string & in) {
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

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        ret += base64_url_alphabet[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      ret += base64_url_alphabet[char_array_4[j]];

    while((i++ < 3))
      ret += '=';

  }
  return ret;
}



typedef unsigned char BYTE;

static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789-_";


static inline bool is_base64(BYTE c) {
  return (isalnum(c) || (c == '-') || (c == '_'));
}

std::vector<BYTE> base64_decode_bin(std::string const& encoded_string) {
  int in_len = encoded_string.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  BYTE char_array_4[4], char_array_3[3];
  std::vector<BYTE> ret;

  while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = base64_chars.find(char_array_4[i]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
          ret.push_back(char_array_3[i]);
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++)
      char_array_4[j] = 0;

    for (j = 0; j <4; j++)
      char_array_4[j] = base64_chars.find(char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
  }

  return ret;
}

static string encryptPath(const string path){
    auto len = sizeof(path.c_str()) + crypto_secretbox_MACBYTES;
    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    unsigned char ciphertext[len];
    //unsigned char result[crypto_secretbox_NONCEBYTES + len];

    crypto_secretbox_keygen(key);
    randombytes_buf(nonce, sizeof nonce);
    crypto_secretbox_easy(ciphertext, (const unsigned char *) path.c_str(), len, nonce, key);

    //memcpy(result, nonce, sizeof(nonce));
    //memcpy(result+sizeof(result), ciphertext, sizeof(ciphertext));

/*     string nonceStr = std::string(reinterpret_cast<const char*>(nonce));
    string cipherTextStr = std::string(reinterpret_cast<const char*>(ciphertext));
    string result = nonceStr + cipherTextStr; */


    string nonce_b64 = base64_encode(nonce, sizeof(nonce));
    string cipher_b64 = base64_encode(ciphertext, sizeof(ciphertext));

    cout << "nonce: " << ciphertext << endl;
    cout << "nonce_b64: " << nonce_b64 << endl;
    cout << "nonce len: " << nonce_b64.length() << endl;
    cout << "cipher_b64: " << cipher_b64 << endl;
    cout << "cipher len: " << cipher_b64.length() << endl;

    return nonce_b64+cipher_b64;
}

static string decryptPath(const string encryptedPath){
/*     unsigned char nonceAndCipher[crypto_secretbox_NONCEBYTES+encryptedPath.length()];
    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    unsigned char ciphertext[encryptedPath.length()]; */

    string nonce_b64 = encryptedPath.substr(0, 32);
    string cipher_b64 = encryptedPath.substr(32);

    std::vector<BYTE> decoded_nonce = base64_decode_bin(nonce_b64);
    std::vector<BYTE> decoded_cipher = base64_decode_bin(cipher_b64);
/*     string decoded_nonce = base64_decode(nonce_b64);
    string decoded_cipher = base64_decode(cipher_b64); */

/*     unsigned char c_nonce = decoded_nonce.c_str();
    const char* c_cipher = decoded_cipher.c_str(); */

    unsigned char* nonce = &decoded_nonce.front();
    unsigned char* cipher = &decoded_cipher.front();

    cout << "nonce: ";
    for(auto c: decoded_cipher){
        cout << c;
    }
    cout << endl;

/*     memcpy(c_nonce, c_decoded_b64, crypto_secretbox_NONCEBYTES);
    memcpy(ciphertext, c_decoded_b64+crypto_secretbox_NONCEBYTES, encryptedPath.length()); */

/*     cout << "nonce: " << nonce << endl;
    cout << "cipherText: " << cipherText << endl; */

    unsigned char decrypted[4096]; // max path length
    if (crypto_secretbox_open_easy(decrypted, cipher, sizeof cipher, nonce, key) != 0) {
        cout << "Message forged" << endl;
    }
    return std::string(reinterpret_cast<const char*>(decrypted));
}

int main(){
    initSodium();
    loadEncryptionKey();

    string a = encryptPath("/home/zach/enclone/test/files2/file1/file4");
    cout << a << endl;
/*     unsigned char decrypted[4096];
    decrypted = decryptPath(a); */

    //cout << a << " length: " << a.length() << endl;
    cout << decryptPath(a) << endl;


}
