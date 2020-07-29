#include <iostream>
#include <sodium.h>
#include <fstream>
#include <stdio.h>

using namespace std;

string hashFile2(const string path){
    int BUF_SIZ = 1024;
    int HASH_SIZ = 64;
    FILE *stream;
    crypto_generichash_state state;
    unsigned char buf[BUF_SIZ], out[HASH_SIZ];
    char hex[(HASH_SIZ * 2) + 1];
    size_t read;

    stream = fopen(path.c_str(), "r");

    crypto_generichash_init(&state, NULL, 0, HASH_SIZ);
    while(!feof(stream)) {
        read = fread(buf, 1, BUF_SIZ, stream);
        crypto_generichash_update(&state,buf,read);
    }
    crypto_generichash_final(&state, out, HASH_SIZ);
    sodium_bin2hex(hex, sizeof hex, out, HASH_SIZ);
    //puts(hex);
    return hex;
}

string hashFile(const string path){
    int BUFFER_SIZE = 512;
    int HASH_SIZE = 64;

    unsigned char out[HASH_SIZE];
    unsigned char buf[BUFFER_SIZE];
    char hex[(HASH_SIZE * 2) + 1];
    size_t read;

    crypto_generichash_state state;
    crypto_generichash_init(&state, NULL, 0, HASH_SIZE);

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
    crypto_generichash_final(&state, out, HASH_SIZE);
    sodium_bin2hex(hex, sizeof hex, out, HASH_SIZE);
    return hex;
}

int main(){
    cout << "hashFile: " << hashFile("/home/zach/enclone/test/test.img") << endl;
    cout << "hashFile2: " << hashFile2("/home/zach/enclone/test/test.img") << endl;
    
}

