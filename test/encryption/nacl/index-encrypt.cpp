#include <iostream>
#include <sodium.h>
#include <fstream>
#include <stdio.h>
#include <cstring>
#include <filesystem>
#include <algorithm> 

// g++ index-encrypt.cpp -o idx -lsodium -lstdc++fs -std=c++17 

using namespace std;
namespace fs = std::filesystem;

unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
uint8_t subkey1[64];

void initSodium(){
    if (sodium_init() != 0) {
        cout << "Encryption: Error initialising libsodium" << endl;
    } else { 
        cout << "Encryption: libsodium initialised" << endl; 
    }
}

int loadEncryptionKey(){
    if(fs::file_size("key") != crypto_secretstream_xchacha20poly1305_KEYBYTES){
        cout << "Error loading encryption key - key size is incorrect" << endl;
        return 1;         
    }
    std::streampos size;
    char memblock[crypto_secretstream_xchacha20poly1305_KEYBYTES];

    std::ifstream file ("key", std::ios::in|std::ios::binary|std::ios::ate);
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

string pwhash_str(string key_b64){
    char hashed_password[crypto_pwhash_STRBYTES];

    string halfSubKey = key_b64.substr(0, key_b64.length()/2); // split the key in half (32 bytes)

    cout << "half of subkey: " << halfSubKey << endl;

    // even if Argon hash is defeated, we only have half a subkey. with a full subkey, we still cannot get the original master key
    
    if (crypto_pwhash_str
    (hashed_password, halfSubKey.c_str(), halfSubKey.length(), crypto_pwhash_OPSLIMIT_SENSITIVE, crypto_pwhash_MEMLIMIT_SENSITIVE) != 0){
        cout << "out of memory" << endl;
    }

    cout << "hashed key str: " << hashed_password << endl;

    auto hashedPasswordStr = std::string(reinterpret_cast<const char*>(hashed_password));
    
    // remove argon2id parameters
    size_t pos = 0;
    int count = 0;

    while(count != 3){
        pos+=1;
        pos = hashedPasswordStr.find("$", pos);
        count++;
    }

    if (pos == std::string::npos){
        cout << "error: Argon2id hashed string, unable to find delimiter" << endl;
    } else {
        hashedPasswordStr = hashedPasswordStr.substr(pos+1);
    }
    
    cout << "hashed key str2: " << hashedPasswordStr << " length: " << hashedPasswordStr.length() << endl;

    auto hashed_b64 = base64_encode(hashedPasswordStr);
    cout << "hashed key b64: " << hashed_b64 << " length: " << hashed_b64.length() << endl;

    return hashed_b64;
}

#define KEY_LEN crypto_box_SEEDBYTES

void pwhash(string key_b64){
    unsigned char salt[crypto_pwhash_SALTBYTES];
    unsigned char new_key[KEY_LEN];
    randombytes_buf(salt, sizeof salt);

    if (crypto_pwhash
        (new_key, KEY_LEN, key_b64.c_str(), key_b64.length(), salt,
        crypto_pwhash_OPSLIMIT_SENSITIVE, crypto_pwhash_MEMLIMIT_SENSITIVE,
        crypto_pwhash_ALG_DEFAULT) != 0) {
        /* out of memory */
    }

    cout << "new key: " << new_key << "new key size: " << sizeof new_key << endl;
    auto key_str = std::string(reinterpret_cast<const char*>(new_key));
    auto salt_str = std::string(reinterpret_cast<const char*>(salt));
    string key_and_salt_str = key_str + salt_str;
    auto new_key_b64 = base64_encode(key_and_salt_str);
    cout << "new_key_b64: " << new_key_b64 << endl;

}

int main(){
    initSodium();
    loadEncryptionKey();

    crypto_kdf_derive_from_key(subkey1, sizeof subkey1, 1, "INDEX___", key);

    // base64 key
    auto key_b64 = base64_encode(std::string(reinterpret_cast<const char*>(key)));
    cout << "master key in b64: " << key_b64 << endl;

    auto subkey_b64 = base64_encode(std::string(reinterpret_cast<const char*>(subkey1)));
    cout << "sub key in b64: " << subkey_b64 << endl;

    string hashed_b64 = pwhash_str(subkey_b64);

    auto decoded_b64 = base64_decode(hashed_b64);
    cout << "decoded b64: " << decoded_b64 << " length: " << decoded_b64.length() << endl;

    string argon_param = "$argon2id$v=19$m=1048576,t=4,p=1$";

    string decodedAndParams = argon_param + decoded_b64; 

    cout << "decodedAndParams: " << decodedAndParams << endl;

    if (crypto_pwhash_str_verify
    (decodedAndParams.c_str(), "cBivDoh6KUrTStfrAWTGMGSoRIVTV34DkWYvq79o3LC", strlen("cBivDoh6KUrTStfrAWTGMGSoRIVTV34DkWYvq79o3LC")) != 0) {
        cout << "wrong password - did not verify" << endl;
    } else {
        cout << "verified password" << endl;
    }

    /* 
    derive subkey
    base64 encode subkey
    use half of base64 subkey as password in Argon2id algorithm to string
    remove Argon2id params from front of string
    base64_url encode Argon2id hash = filename of index

    to restore index:
        - compute same subkey from master key
        - base64 encode subkey split in half
        - decode base 64 for all filenames on remote
        - prefix Argon2id params to all decoded b64 strings ("$argon2id$v=19$m=1048576,t=4,p=1$")
        - use verify function to check base 64 encoded half subkey against all values until it verifies
        - on average will have to check half the files before finding a match. base64 decode is quick, test verification.

        - 
     */
}
