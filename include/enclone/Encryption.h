#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

#include <openssl/sha.h>

using std::string;
using std::cout;
using std::endl;

class Encryption{
    private:

    public:
        virtual ~Encryption() = 0; // pure virtual - class is abstract

        static string sha256(const string str);
};

#endif