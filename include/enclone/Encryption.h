#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <random>
#include <fstream>
#include <cstring>

#include <sodium.h>

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

        static std::string base64_encode(const std::string &in);
        static std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len);
        static std::string base64_decode(const std::string &in);

        static constexpr char base64_url_alphabet[64] = {
            'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
            'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
            'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
            'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '_'
        };

        static std::string passwordKDF(string password);
        static bool verifyPassword(string hash, string password);

        static string deriveKey(string password); // derive a key from a password (subkey in b64 used as password), using a random salt
        static string deriveKey(string password, string salt_b64); // derive a key from password, using a specified salt
        static bool verifyKey(string password, string saltedKey_b64);

        static const int getRandomFilenameLength();
    
    private:
        // encryption/decryption constants
        static const int CHUNK_SIZE = 4096;

        // random path/filename constants
        static const int RANDOM_FILENAME_LENGTH = 88;

        // file hash constants
        static const int BUFFER_SIZE = 512;
        static const int FILE_HASH_SIZE = 64;

        static string randomString(std::size_t length);
};

#endif