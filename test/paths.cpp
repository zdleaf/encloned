#include <iostream>
#include <string>

using namespace std;

void split(string &path){
    string downloadDir = "/home/zach/enclone/dl"; // the directory we want to download to
    std::size_t found = path.find_last_of("/");
    string dirPath = downloadDir + path.substr(0,found+1); // put 
    string fileName = path.substr(found+1);
    cout << "path is: " << dirPath << ", fileName is: " << fileName << endl;
    cout << "complete is: " << dirPath + fileName << endl;
}

int main(){

    string p1 = "/home/zach/enclone/tmp2/subdir/subfile";
    string p2 = "/home/zach/enclone/tmp2/file1";

    split(p1);
    split(p2);

}