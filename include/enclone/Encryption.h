#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <random>

#include <openssl/sha.h>

using std::string;
using std::cout;
using std::endl;

class Encryption{
    private:
        static string sha256(const string str);
        static string randomString(std::size_t length);

    public:
        virtual ~Encryption() = 0; // pure virtual - class is abstract
        static string hashPath(const string path);

};

#endif