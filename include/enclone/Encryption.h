#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <random>

#include <sodium.h>
#include <openssl/sha.h>

#define CHUNK_SIZE 4096 // libsodium file chunk size

using std::string;
using std::cout;
using std::endl;

class Encryption{
    private:
        static string sha256(const string str);
        static string randomString(std::size_t length);
        
    public:
        virtual ~Encryption() = 0; // pure virtual - class is abstract
        static void initSodium();

        static string hashPath(const string path);

        static int encryptFile(const char *target_file, const char *source_file, const unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES]);
        static int decryptFile(const char *target_file, const char *source_file, const unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES]);

};

#endif