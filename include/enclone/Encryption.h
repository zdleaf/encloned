#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <random>
#include <fstream>

#include <sodium.h>
#include <openssl/sha.h>

using std::string;
using std::cout;
using std::endl;

class Encryption{
    public:
        virtual ~Encryption() = 0; // pure virtual - class is abstract
        static void initSodium();

        static string hashPath(const string path);
        static string hashFile(const string path);

        static int encryptFile(const char *target_file, const char *source_file, const unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES]);
        static int decryptFile(const char *target_file, const char *source_file, const unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES]);
    
    private:
        // encryption/decryption constants
        static const int CHUNK_SIZE = 4096;

        // path/filename hash constants
        static const int PATH_HASH_SALT_LENGTH = 128;

        // file hash constants
        static const int BUFFER_SIZE = 512;
        static const int FILE_HASH_SIZE = 64;

        static string sha256(const string str);
        static string randomString(std::size_t length);
};

#endif