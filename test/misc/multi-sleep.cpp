#include <iostream>
#include <string>
#include <chrono>
#include <thread>

// g++ multi-sleep.cpp -o slp -std=c++17 

using namespace std;

void scanFileChange(){
    cout << "scanFileChange" << endl;
}
void execQueuedSQL(){
    cout << "execQueuedSQL" << endl;
}
void indexBackup(){
    cout << "indexBackup" << endl;
}

int main(){
    while(true){
        //cout << "Watch: Scanning for file changes..." << endl; cout.flush(); 
        for(int i = 0; i<5; i++){
            for(int i = 0; i<5; i++){
                scanFileChange();
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            execQueuedSQL();
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        indexBackup();
        //std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

