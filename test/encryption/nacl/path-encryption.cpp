#include <iostream>
#include <sodium.h>
#include <fstream>
#include <stdio.h>
#include <cstring>
#include <filesystem>

// g++ path-encryption.cpp -o path -lsodium -lstdc++fs -std=c++17 

using namespace std;
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

std::string base64_encode(std::string bin_str){
    const size_t bin_len = bin_str.size();
    const size_t  base64_max_len = sodium_base64_encoded_len(bin_len, BASE64_VARIATION);
    std::string base64_str(base64_max_len-1,0);

    char * encoded_str_char = sodium_bin2base64(
            base64_str.data(),
            base64_max_len,
            (unsigned char *) bin_str.data(),
            bin_len,
            BASE64_VARIATION
    );

    if(encoded_str_char == NULL){
        throw "Base64 Error: Failed to encode string";
    }

    return base64_str;
}

std::string base64_decode(std::string base64_str){
    unsigned char * bin;
    const size_t bin_maxlen = 4096;
    const size_t b64_len = base64_str.size();

/*     int sodium_base642bin(bin, 
                        bin_maxlen,
                        (unsigned char *) base64_str.data(), 
                        b64_len,
                        const char * const ignore, 
                        size_t * const bin_len,
                        NULL, 
                        BASE64_VARIATION); */
}

static string encryptPath(const string path){
    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    unsigned char ciphertext[path.length()];

    crypto_secretbox_keygen(key);
    randombytes_buf(nonce, sizeof nonce);
    crypto_secretbox_easy(ciphertext, (const unsigned char *) path.c_str(), path.length(), nonce, key);

    string nonceStr = std::string(reinterpret_cast<const char*>(nonce));
    string cipherTextStr = std::string(reinterpret_cast<const char*>(ciphertext));
    string result = nonceStr + cipherTextStr;

/*     cout << "nonce: " << nonceStr << endl;
    cout << "sizeof nonce: " << sizeof nonceStr << endl;
    cout << "cipherText: " << cipherTextStr << endl;
    cout << "result: " << result << endl; */
    cout << "b64: " << base64_encode(result) << endl;
    return result;
}

static string decryptPath(const string encryptedPath){
    string nonce = encryptedPath.substr(0, 30);
    string cipherText = encryptedPath.substr(30);

/*     cout << "nonce: " << nonce << endl;
    cout << "cipherText: " << cipherText << endl; */


    unsigned char decrypted[4096]; // max path length
    if (crypto_secretbox_open_easy(decrypted, (const unsigned char *) cipherText.c_str(), cipherText.length(), (const unsigned char *) nonce.c_str(), key) != 0) {
        cout << "Message forged" << endl;
    }
    return std::string(reinterpret_cast<const char*>(decrypted));
}

int main(){
    initSodium();
    loadEncryptionKey();

    string a = encryptPath("/home/zach/enclone/test/files2/file1");
    //cout << a << " length: " << a.length() << endl;
    cout << decryptPath(a) << endl;


}
