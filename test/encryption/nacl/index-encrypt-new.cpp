#include <iostream>
#include <sodium.h>
#include <fstream>
#include <stdio.h>
#include <cstring>
#include <filesystem>
#include <algorithm> 

// g++ index-encrypt-new.cpp -o idx -lsodium -lstdc++fs -std=c++17 

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

std::string new_base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
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

void deriveKey(string key_b64){
    unsigned char salt[crypto_pwhash_SALTBYTES+2];
    unsigned char newKey[48];

    randombytes_buf(salt, sizeof salt);

    if (crypto_pwhash
        (newKey, sizeof newKey, key_b64.c_str(), key_b64.length(), salt,
        crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE,
        crypto_pwhash_ALG_DEFAULT) != 0) {
        /* out of memory */
    }

    auto newKey_b64 = new_base64_encode(newKey, sizeof newKey);
    auto salt_b64 = new_base64_encode(salt, sizeof salt);

    cout << "newKey_b64: " << newKey_b64 << " length: " << newKey_b64.length() << " key size: " << sizeof newKey << endl;
    cout << "salt_b64: " << salt_b64 << " length: " << salt_b64.length() << " salt size: " << sizeof salt << endl;
}

string deriveKeyFromSalt(string key_b64, string salt_b64){
    unsigned char salt[crypto_pwhash_SALTBYTES+2]; // so resulting b64 encode is 24 chars
    unsigned char newKey[48]; // b64 encode of this will be 64 chars
    
    string saltStr = base64_decode(salt_b64); // get the binary data from salt
    strcpy((char*) salt, saltStr.c_str());
    //randombytes_buf(salt, sizeof salt)

    if (crypto_pwhash
        (newKey, sizeof newKey, key_b64.c_str(), key_b64.length(), salt,
        crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE,
        crypto_pwhash_ALG_DEFAULT) != 0) {
        /* out of memory */
    }

    auto newKey_b64 = new_base64_encode(newKey, sizeof newKey);
    //auto salt_b64 = new_base64_encode(salt, sizeof salt);

    cout << "newKey_b64: " << newKey_b64 << " length: " << newKey_b64.length() << " key size: " << sizeof newKey << endl;
    cout << "salt_b64: " << salt_b64 << " length: " << salt_b64.length() << " salt size: " << sizeof salt << endl;
    return newKey_b64;
}


int main(){
    initSodium();
    loadEncryptionKey();

    crypto_kdf_derive_from_key(subkey1, sizeof subkey1, 1, "INDEX___", key);

    vector<string> vec;
    vec.push_back("wbNINpYwtYbwaK2Dg51v7POjdTRwBdXyD5TCrjwdslMStlAvfIaNINTSn8j5c6u2Zs3U9sUfhUjsMpiR2rjf80FP");
    vec.push_back("x11wyRKqhkaZx7WzcwaZfJXZ1gqrjxxqPm8gnUBWPJiCIuSSuiyPFJ8cwIEf62vZAH3fl9pe3uO1yx15m7BE5LG7");
    vec.push_back("xKzqz7btFi5VXWAVb5G3srF7zonBR0Owkajnc9YgTdBHFhd983fO0Khv31oIxp7jNtgrBeioK6kXR4DX4Na7pIfZ");
    vec.push_back("vGWCOVZtmaCFooLXph2s00W3FtTu4pkJrAqpXcaUVpNVHJyiEx7vXmgZK6ELk9QSmBwUYR89EhGQrG5AchEnabGN");
    vec.push_back("uAAV8mzeAWEwx8LS5L66kKvoxFFIO4FItljgF8iAZUrGyXfvxcs3f43z0jzyiQGhMbIE2lV2zC6ZiaEc8xSudp8t");
    vec.push_back("sZ42vINY8Iwz7hZD0o9685AINa5ATj7yTIO2jjv5nRFJFIVGKhCxa711MsmbqvfMWgzr2Tjd5d6ibXxcwAm8r0Tl");
    vec.push_back("rSZZcL2ShGiUgdBI7ZDBJI6hyU4uFHnOTtrBNANy5gwE40YofiRFaV4vUje8I2S5hDBX2YkMvaezDmbuOM3KyTt0");
    vec.push_back("qM7JBetmusTQSSEG56jeciFf9jy0DzbwCx34Z6CI4LKHSr19SInwIEpIiD9CfNpBLLiHJghCDYz4tHGFqOvJJG83");
    vec.push_back("pXmSe11MSwbquZMIdjCZTztc2aWyad2cYHXSDNNNOmmk2S5QbxWqYJzMnh37yYzLr07xujOBnNAKZjh58E5uH8kC");
    vec.push_back("oVwSmR8dUUh8d1SI2a10Vep6UJivcvg88sUFrPcK5uFPf6FRedPX6VNMsvIEmHGeDDEyjs2x1N1VYQhI0Mxsv1fO");
    vec.push_back("n9amQ6bZ7IAzL2Bu6WFfvhtBRU9pSEc2DPfpXuXAVUd16iSX89R8ihx2gmUq39QxQl6BL0riU9if4fnq13ziuGfd");
    vec.push_back("0AFqKN-EpumhZa1n4yH97TVC1x_9N8kheJAeFPyfCujTL1xmd4mgkz_jki3PcMfwy_CVhUynrI7wxGUe-PWLQZVa");

    // base64 key
    auto key_b64 = base64_encode(std::string(reinterpret_cast<const char*>(key)));
    cout << "master key in b64: " << key_b64 << endl;

    auto subkey_b64 = base64_encode(std::string(reinterpret_cast<const char*>(subkey1)));
    cout << "sub key in b64: " << subkey_b64 << endl;

    //deriveKey(subkey_b64);

    for(auto item: vec){
        string key = item.substr(0, 64);
        string salt = item.substr(64);
        
        if(deriveKeyFromSalt(subkey_b64, salt) == key){
            cout << "verified - key: " << key << " salt: " << salt << endl;
        }
    }

    

    /* 
    to derive index filename:
        - derive subkey
        - base64 encode subkey
        - use b64 encoded subkey as a password to derive another key, with a random salt
        - store resulting b64 of new key + b64 random salt = 88 char filename

    to restore index:
        - compute same subkey from master key
        - base64 encode subkey
        - decode b64 for last 24 chars of all filenames on remote - these are potential salt values
        - use subkey and decoded b64 possible salt, to derive the newkey. base64 new key and compare with first 64 chars of file name.
     */
}
