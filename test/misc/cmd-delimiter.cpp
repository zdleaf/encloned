#include <iostream>
#include <string>

using namespace std;

int main(){
    string response;
    string delimiter = "|";

    std::string request = "restore|dl|cc08388cd19f894b12ddddfb3e4d24851edd553c3f24342c92f63938ee1fcb98";

    auto firstDelimiter = request.find(delimiter);
    auto secondDelimiter = request.find(delimiter, firstDelimiter+1);
    std::string cmd = request.substr(0, firstDelimiter); 
    std::string arg1 = request.substr(firstDelimiter+1, secondDelimiter-firstDelimiter-1);
    std::string arg2 = request.substr(secondDelimiter+1);

    // DEBUGGING
    cout << "Full request is: \"" << request << "\"" << endl;
    cout << "Command is: \"" << cmd << "\"" << endl; // get command that appears before delimter
    cout << "Arg1 is: \"" << arg1 << "\"" << endl;
    cout << "Arg2 is: \"" << arg2 << "\"" << endl;
}